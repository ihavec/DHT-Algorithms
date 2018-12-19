#!/usr/local/bin/python
#coding = utf-8

'''
[input]
hosts
192.168.1.1 domain1 domain2
192.168.1.2 domain1

[output]
{'domain2': ['102.168.1.1'], 'domain1': ['102.168.1.1', '192.168.1.2']}
{'102.168.1.1': ['domain2', 'domain1'], '192.168.1.2': ['domain1']}
102.168.1.1  domain2 domain1

192.168.1.2  domain1

'''
# 1. hosts ->  dict{domain:[ips]}
HOSTS_CONFIG_FILE = "./hosts"
file_in = open(HOSTS_CONFIG_FILE)
rline = file_in.readline()
domain_dict = {}
while rline:
    line_items = rline.split()
    if len(line_items) > 1:
        ip = line_items[0]
        domains = line_items[1:]
        for domain in domains:
          if domain not in domain_dict:
              domain_dict[domain] = []
          domain_dict[domain].append(ip)
    rline = file_in.readline()
file_in.close()
print domain_dict

# 2.  dict{group:[ips]} -> dict{ip:[groups]}
ip_dict = {}
for domain, ips in domain_dict.items():
    for ip in ips:
        if ip not in ip_dict:
            ip_dict[ip] = []
        ip_dict[ip].append(domain)
print ip_dict

# 3. dict{ip:[groups]} -> output
HOSTS_OUTPUT = "./output"
file_out = open(HOSTS_OUTPUT,"w")
for ip, groups in ip_dict.items():
    group_str =""
    for group in groups:
        group_str = group_str + " "+ group
    wline = ip + " "+ group_str + "\n"
    print wline
    file_out.writelines(wline)
file_out.close()
