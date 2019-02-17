/*
1. 说明 
以下代码把crush hash_rjenkins和straw算法最核心代码提取出来演示，
把文件数据保存到ceph集群分为以下两步：
 1) hash(object_name) -> pg, 把文件使用hash_rjenkins算法选择pg, pg类似一致性hash算法里的虚拟节点;
 2) straw(pg) -> osd, 使用straw算法选择定位osd硬盘设备,osd的权重越大随机被挑中的概率越大.

2. 编译和测试
gcc crush.c -o crush
./crush 
	ceph_str_hash_rjenkins('object1') = 0xbac5debc(4)
	pool 'pool1' (18) object 'object1' -> pg 18.bac5debc(18.4) -> up ([2,0,1])
	
3. hash_rjenkins和straw算法伪代码
	locator = object_name
	obj_hash = hash(locator) #此处为ceph_str_hash_rjenkins法
	pg = obj_hash % num_pg
	OSDs_for_pg = crush(pg)  #多次调用bucket_straw_choose返回多个副本
	primary = osds_for_pg[0]
	replicas = osds_for_pg[1:]

	def crush(pg):  
	   all_osds = ['osd.0', 'osd.1', 'osd.2', ...]
	   result = []
	   # size is the number of copies; primary+replicas
	   while len(result) < size:
	       r = hash(pg)
	       chosen = all_osds[ r % len(all_osds) ]
	       if chosen in result: #直到选出多个不同的OSD
	           # OSD can be picked only once
	           continue
	       result.append(chosen)
	   return result
*/
#include <stdio.h>
#include <time.h>
#include <string.h>

typedef unsigned int __u32;
#define osd_max_num (3)

/* ======================== hash_rjenkins 算法 ======================== */
/*
 * Robert Jenkin's hash function.
 * http://burtleburtle.net/bob/hash/evahash.html
 * This is in the public domain.
 */
#define mix(a, b, c)						\
	do {							\
		a = a - b;  a = a - c;  a = a ^ (c >> 13);	\
		b = b - c;  b = b - a;  b = b ^ (a << 8);	\
		c = c - a;  c = c - b;  c = c ^ (b >> 13);	\
		a = a - b;  a = a - c;  a = a ^ (c >> 12);	\
		b = b - c;  b = b - a;  b = b ^ (a << 16);	\
		c = c - a;  c = c - b;  c = c ^ (b >> 5);	\
		a = a - b;  a = a - c;  a = a ^ (c >> 3);	\
		b = b - c;  b = b - a;  b = b ^ (a << 10);	\
		c = c - a;  c = c - b;  c = c ^ (b >> 15);	\
	} while (0)

unsigned ceph_str_hash_rjenkins(const char *str, unsigned length)
{
	const unsigned char *k = (const unsigned char *)str;
	__u32 a, b, c;  /* the internal state */
	__u32 len;      /* how many key bytes still need mixing */

	/* Set up the internal state */
	len = length;
	a = 0x9e3779b9;      /* the golden ratio; an arbitrary value */
	b = a;
	c = 0;               /* variable initialization of internal state */

	/* handle most of the key */
	while (len >= 12) {
		a = a + (k[0] + ((__u32)k[1] << 8) + ((__u32)k[2] << 16) +
			 ((__u32)k[3] << 24));
		b = b + (k[4] + ((__u32)k[5] << 8) + ((__u32)k[6] << 16) +
			 ((__u32)k[7] << 24));
		c = c + (k[8] + ((__u32)k[9] << 8) + ((__u32)k[10] << 16) +
			 ((__u32)k[11] << 24));
		mix(a, b, c);
		k = k + 12;
		len = len - 12;
	}

	/* handle the last 11 bytes */
	c = c + length;
	switch (len) {            /* all the case statements fall through */
	case 11:
		c = c + ((__u32)k[10] << 24);
	case 10:
		c = c + ((__u32)k[9] << 16);
	case 9:
		c = c + ((__u32)k[8] << 8);
		/* the first byte of c is reserved for the length */
	case 8:
		b = b + ((__u32)k[7] << 24);
	case 7:
		b = b + ((__u32)k[6] << 16);
	case 6:
		b = b + ((__u32)k[5] << 8);
	case 5:
		b = b + k[4];
	case 4:
		a = a + ((__u32)k[3] << 24);
	case 3:
		a = a + ((__u32)k[2] << 16);
	case 2:
		a = a + ((__u32)k[1] << 8);
	case 1:
		a = a + k[0];
		/* case 0: nothing left to add */
	}
	mix(a, b, c);

	return c;
}

/* ======================== straw 算法 ======================== */
/*
 * Robert Jenkins' function for mixing 32-bit values
 * http://burtleburtle.net/bob/hash/evahash.html
 * a, b = random bits, c = input and output
 */
#define crush_hashmix(a, b, c) do {			\
		a = a-b;  a = a-c;  a = a^(c>>13);	\
		b = b-c;  b = b-a;  b = b^(a<<8);	\
		c = c-a;  c = c-b;  c = c^(b>>13);	\
		a = a-b;  a = a-c;  a = a^(c>>12);	\
		b = b-c;  b = b-a;  b = b^(a<<16);	\
		c = c-a;  c = c-b;  c = c^(b>>5);	\
		a = a-b;  a = a-c;  a = a^(c>>3);	\
		b = b-c;  b = b-a;  b = b^(a<<10);	\
		c = c-a;  c = c-b;  c = c^(b>>15);	\
	} while (0)

#define crush_hash_seed 1315423911

/* 
返回一个随机数,参数不变返回结果也一样.
参数1: pg_id 
参数2: osd_id
参数3: 第c个副本
 */
static __u32 crush_hash32_rjenkins1_3(__u32 a, __u32 b, __u32 c)
{
    __u32 hash = crush_hash_seed ^ a ^ b ^ c;
    __u32 x = 231232;
    __u32 y = 1232;
    crush_hashmix(a, b, hash);
    crush_hashmix(c, x, hash);
    crush_hashmix(y, a, hash);
    crush_hashmix(b, x, hash);
    crush_hashmix(y, c, hash);
    return hash;
}

/*
straw算法,osd的权重越大,随机被挑中的概率越大.
1. crush_hash32_rjenkins1_3( pg, osd_id, r ) ===> draw
2. ( draw &0xffff ) * osd_weight ===> osd_straw
3. pick up high_osd_straw
*/
int bucket_straw_choose(int x,int r)
{
	int i;
	unsigned int osd_weight = 1;
	int osd[osd_max_num] = {0,1,2}; //osd编号
	int high = 0;
	unsigned long long high_draw = 0;
	unsigned long long draw;
	for (i = 0; i < osd_max_num; i++)
	{
	    draw = crush_hash32_rjenkins1_3(x, osd[i], r);
	    draw &= 0xffff;
	    draw *= osd_weight;
	    (CRUSH_HASH & 0xFFFF) * weight
	    
	    //选出CRUSH_HASH(pg,osd_id,r)&0xFFFF)*osd_weight乘积最大的osd赋给high
	    if (i == 0 || draw > high_draw)
	    {
	        high = i; 
	        high_draw = draw;
	    }
	}
	return high;
}

/* ======================== hash_rjenkins和straw测试 ======================== */
int main()
{
	char pool_name[] = "pool1";
	int pool_id = 18;
	
	/* 1. object_name -> pg, hash_rjenkins算法选择pg,pg类似一致性hash选择定位虚拟节点 */
	int num_pg = 8;
	char object_name[] = "object1";
	unsigned int obj_hash = ceph_str_hash_rjenkins(object_name, strlen(object_name));
	int pg = obj_hash % num_pg;
	printf("ceph_str_hash_rjenkins('%s') = 0x%x(%d)\n",object_name,obj_hash,pg);
	
	/* 2. pg -> osd, straw算法选择osd,osd的权重越大随机被挑中的概率越大 */
	int primary,replicas1,replicas2;
	primary = bucket_straw_choose(obj_hash,1);
	replicas1 = bucket_straw_choose(obj_hash,2);
	replicas2 = bucket_straw_choose(obj_hash,3);
	printf("pool '%s' (%d) object '%s' -> pg %d.%x(%d.%d) -> up ([%d,%d,%d])\n",
		pool_name,pool_id,object_name,pool_id,obj_hash,pool_id,pg,primary,replicas1,replicas2);
}