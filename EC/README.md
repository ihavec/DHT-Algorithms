
# 纠删码算法原理

## 1. 原始数据切片

### 原始数据”ABCDEFGHIJKLMNOP”被切分成4个部分，矩阵表示：
![image](https://github.com/larkguo/Algorithms/EC/data/原始数据矩阵.png)

## 2. 编码矩阵

###eed-Solomon算法创建一个编码矩阵，您可以将其与数据矩阵相乘以创建编码数据。设置矩阵，使得结果的前四行与输入的前四行相同。这意味着数据保持不变，而它真正做的就是计算奇偶校验。结果是一个矩阵，其行数比原始行多两奇偶校验行.
![image](https://github.com/larkguo/Algorithms/EC/data/原始编码矩阵.png)

## 3. 数据丢失

### 编码矩阵的每一行产生一行结果。因此编码矩阵的每一行产生文件的结果片段之一（So each row of the coding matrix makes one of the resulting pieces of the file. ）。因为行是独立的，所以可以划掉两行，并且等式仍然成立。
![image](https://github.com/larkguo/Algorithms/EC/data/丢失数据.png)

得到:
![image](https://github.com/larkguo/Algorithms/EC/data/丢失数据后的矩阵关系.png)

## 4. 逆矩阵

### 我们知道编码矩阵，左边的矩阵是可逆的。存在逆矩阵，当乘以编码矩阵时，产生单位矩阵（解码矩阵）。
两边都乘以现有编码矩阵的逆矩阵（解码矩阵）:
![image](https://github.com/larkguo/Algorithms/EC/data/得到逆矩阵.png)

## 5. 恢复原始数据

### 与基本代数一样，在矩阵代数中，你可以将方程的两边乘以相同的东西。在这种情况下，我们将在左侧乘以单位矩阵. 
左边消掉现有编码矩阵和其逆矩阵（解码矩阵）:
![image](https://github.com/larkguo/Algorithms/EC/data/消逆矩阵和解码矩阵.png)

恢复原始数据，原始数据=现有编码矩阵的逆矩阵（解码矩阵）*现有数据矩阵：
![image](https://github.com/larkguo/Algorithms/EC/data/恢复原始数据.png)

因此，为了制作解码矩阵，过程是采用原始编码矩阵，将缺失部分的行划掉，然后找到逆矩阵。然后，您可以将逆矩阵与现有数据矩阵相乘。
下图演示对test原始数据进行4切片，2校验，共生成6份文件test.N，删除test,test.1和test.4后进行恢复：
![image](https://github.com/larkguo/Algorithms/EC/data/debug.png)


以上纠删码算法实现见 https://github.com/klauspost/reedsolomon
编码数据验证代码见[encode.go](https://github.com/larkguo/Algorithms/EC/encode.go)
恢复数据验证代码[decode.go](https://github.com/larkguo/Algorithms/EC/decode.go)
原理参考 https://www.backblaze.com/blog/reed-solomon/


