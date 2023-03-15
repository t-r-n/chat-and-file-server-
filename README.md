# chat-and-file-server-

# 文件和消息转发服务端和qt搭建的网络聊天室客户端
* 文件和消息转发服务端使用boost::asio搭建 
* 消息转发服务器运行在两核一兆带宽下1000个并发连接无延迟
mes_server文件夹下是消息转发服务端   
file_server文件夹下是文件转发服务端   
均是异步proactor模型的网络服务端  
* 聊天室客户端使用qt搭建     
clint文件夹下是客户端qt项目   
* testServer文件夹下为压力测试程序
## 实现功能：
* 账号注册,登录，向已注册账号发送消息支持向离线账号发送消息待上线接收  
* 向已注册账号发送文件,(将文件发送至服务端，通知对方是否接受该文件)   
* 创建聊天室，其他账号通过聊天室账号加入到同一个聊天室聊天    
* 添加数据库连接池保存账号密码

## 编译环境   
服务端：vs2019 boost1.79  
客户端 qt5.14.2  
## 压力测试
服务端跑在2核/2gb/4mbps带宽下
测试1000个线程随机收发，基本无延迟

#### linux环境：更改server_config.h 头文件下宏定义 为#LINUX 注释#WIN
### 消息服务器编译(需要安装boost1.79库)
g++ -o mesServer *.h *.cpp -pthread
### 消息服务器运行 
sudo nohup ./mesServer ./ >output &



## 参数
客户端 服务器ip地址 端口号  默认(127.0.0.1)  
mesServer 文件服务器下filerecv的绝对路径如：E:/fin_server/file_server/trfileserver/FileRecv/*   
file_server ip地址 端口号 文件缓冲区大小  (一般是100)
