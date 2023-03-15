#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

//#define DEBUG
//#define LOCK_DEBUG
//#define LINUX
//#define COMYSQL
#define WIN

#include <iostream>
#include<boost/asio.hpp>
#include<vector>
#include<string>
#include<memory>
#include<mutex>
#include<thread>
#include<boost/bind.hpp>
#include<unordered_map>
#include<queue>
#include<list>
#ifdef WIN
#include <stdio.h>
#include <io.h>
#endif //WIN
#ifdef LINUX
#include<stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#endif
#include"thread_pool.h"     
#include<fstream>
#include<boost/asio/strand.hpp>
#include<list>
#include<future>

using namespace std;
using namespace boost::asio;

    enum reback {
        readerror = 3,
        writeerror = 4,
        passerror = 5,       //密码错误
        linkerror = 6,    //连接断开等
        login = 7,        //已经在线
        logsucc = 8,    //登陆成功
        loginsucc = 9,  //注册成功
        loginfal = 10,
        fitrfail = 11,  //文件传输失败
        fitrbusy = 12,    //文件空闲服务队列已满
        firetrst = 13,
        sendSucc=15,
        BuildTalkRoomSuc=16
    };
    extern  bool is_have_task;
    extern  mutex my_mutex;
    extern  condition_variable semu_cond;

    extern  bool is_have_task1;
    extern  mutex my_mutex1;
    extern  condition_variable semu_cond1;
#pragma pack(1)  //预编译时字节不对齐
    struct Head {
        unsigned char type;//小文件f：如果packid是-1，-1的包不传消息包后面跟上文件名，说明是一个新文件，起一个file类用智能指针管理，vec把每个类串起来,id是索引，传完删掉该位置指针下一个接着用固定ve容量也就是id容量，起一个文件类queue池管理空闲id
        unsigned int length;
        unsigned int id;//如果是文件类型则把数据传给相应的id
        int          packid;//暂时做用户名长度字段 包=包头+文本+用户名
        unsigned int account;//账号
        unsigned int mima;//密码
        unsigned int sendto;//0给服务器 非0即要发给的账户
        unsigned int status = 0; //0初始状态(server线程开始处理) 1//已处理(server直接跳过) //2//消息可丢弃(serer将该消息出队) packid为-1时表示文件大小
    };
#pragma pack()  //恢复字节对齐
    //const int sizeofhead = sizeof(Head);
    const int sizehead = sizeof(Head);
    struct clintchar {//全局
        queue<std::string>remessage;//待接收消息队列
        queue<std::string>semessage;//待发送消息队列
        mutex semu;  //发送消息锁
        mutex remu;  //接收消息的锁

        vector<int>friend_queue;//群号过来的消息该客户有这个群号就往里发送消息
        std::string name;//后面添加昵称/账号加好友功能//或者不整好友功能把所有服务器有的用户都发过去
        queue<std::string>filequ;//文件队列//待转发（0 file）或待接受（1 file）的文件

        unsigned int account;//账号
        unsigned int password;//密码
        vector<std::string>personalfile;//私人保存在服务器云端的文件
        bool islogin = false;//是否在线
        vector<int>talkRooms;//聊天室
    };
    enum { MAXSIZE = 40 * 1024 + 64 };

    struct clint;
extern    unordered_map<int, std::shared_ptr<clintchar>> account;  //用accountid索引的account集合
extern    mutex acc_mutex;
extern    unordered_map<int, std::shared_ptr<clint>> cur_account_ptr;
extern    mutex cur_account_ptr_mutex;


extern    unordered_map<int, bool>islogin;//在线用户指针速查
    //客户端那边传文件应该另起一个线程不影响主线程通信

    struct talkgroupchar {//群聊消息 //该功能应该还得往消息头里加入发送对象要发送给的群组，clintchar里要保存所有的群组的id
        queue<std::string>remessage;//待接收消息队列
        queue<std::string>semessage;//待发送消息队列
        mutex semu;  //正在往发送消息队列里填消息
        mutex remu;  //接收消息的锁//服务线程用这两个队列的时候先被复制这两个互斥量然后加锁填数据如果没拿到锁会读到那看看之后优化成非阻塞模式可不可行
                     //拿到锁后还得再看一遍队列是否还有数据有可能之前的on_write已经读完了
        vector<std::string>mesage;//储存群内所有的消息
        vector<std::string>friend_queue;  //群内拥有成员数
        std::string name;//后面添加昵称/账号加好友功能//或者不整好友功能把所有服务器有的用户都发过去
        queue<std::string>filequ;//文件队列//待转发（0 file）或待接受（1 file）的文件
        unsigned int id;//群号
        unsigned int password;
        vector<std::string>file;//群文件
        int status = false; //群聊状态，存在，待删除
    };

extern unordered_map<int, queue<std::string>>handleingque;
extern mutex handle_acc_mutex;
    //extern shared_ptr<Server>Se_ptr;
#ifdef COMYSQL
extern queue<std::string>sqlList;
extern mutex sqlList_mu;
#endif


//聊天室
extern int curMaxIndex;
extern unordered_map<int, vector<unsigned int>>talkRoomQueue;
extern mutex talkRoomQueue_mutex;


extern char* filePath;
#endif