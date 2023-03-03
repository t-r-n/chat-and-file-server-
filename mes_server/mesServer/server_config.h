#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

//#define DEBUG
#define LOCK_DEBUG
//#define LINUX
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
#include"thread_pool.h"             //只能添加不同函数和静态成员函数
#include<fstream>
#include<boost/asio/strand.hpp>
//#include<boost/thread.hpp>//包含该lib库方法属性-》连接器-》输入-》附加依赖项E:\boost_1_79_0\boost_1_79_0\bin.v2\libs\thread\build\msvc-14.2\debug\address-model-32\link-static\threadapi-win32\threading-multi\libboost_thread-vc142-mt-gd-x32-1_79.lib
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
        BuildTalkRoomSuc=14
    };
#pragma pack(1)  //预编译时字节不对齐
extern  bool is_have_task;
extern  mutex my_mutex;
extern  condition_variable semu_cond;

extern  bool is_have_task1;
extern  mutex my_mutex1;
extern  condition_variable semu_cond1;
    struct Head {
        unsigned char type;//小文件f：如果packid是-1，-1的包不传消息包后面跟上文件名，说明是一个新文件，起一个file类用智能指针管理，vec把每个类串起来,id是索引，传完删掉该位置指针下一个接着用固定ve容量也就是id容量，起一个文件类queue池管理空闲id
        unsigned int length;
        unsigned int id;//如果是文件类型则把数据传给相应的id
        int          packid;//描述如果是文件是第几个数据包保证客户端若是多线程文件传输的顺序性
        unsigned int account;//账号
        unsigned int mima;//密码
        unsigned int sendto;//0给服务器 非0即要发给的账户
        unsigned int status = 0; //0初始状态(server线程开始处理) 1//已处理(server直接跳过) //2//消息可丢弃(serer将该消息出队) packid为-1时表示文件大小
    };
#pragma pack()  //恢复字节对齐
    //const int sizeofhead = sizeof(Head);
    const int sizehead = sizeof(Head);
    struct clintchar {//全局  //到时候好好想想和clint内部消息队列的锁的阻塞和非阻塞问题，//****************************************************************************888
                               //设置阻塞和非阻塞//全局应该非阻塞轮询各个clint的消息队列，加着锁就询问下一个，否则开始处理，甚至可以
                               //用线程池，一个线程轮询的时候遇到有用户消息要处理就开始处理,处理完开始睡眠等待被唤醒，然后唤醒一个睡眠的线程开始往后轮询，
                               // 轮询的时候又遇到整在处理任务的线程可以跳过去
                                //这样就总有一个线程在轮询
                                //应该是两个线程池，一个是现在这个轮询私人用户消息队列有没有消息的
                                //还有一个线程池是转发消息的,这部分实现滞后先搞完最基本的文件传输再说
                                //只有收到客户端成功的反馈才从队列pop掉消息
        queue<string>remessage;//待接收消息队列
        queue<string>semessage;//待发送消息队列
        mutex semu;  //正在往发送消息队列里填消息
        mutex remu;  //接收消息的锁//服务线程用这两个队列的时候先被复制这两个互斥量然后加锁填数据如果没拿到锁会读到那看看之后优化成非阻塞模式可不可行//拿到锁后还得再看一遍队列是否还有数据有可能之前的on_write已经读完了
        //condition_variable semu_cond;

        vector<int>friend_queue;//群号过来的消息该客户有这个群号就往里发送消息
        string name;//后面添加昵称/账号加好友功能//或者不整好友功能把所有服务器有的用户都发过去
        queue<string>filequ;//文件队列//待转发（0 file）或待接受（1 file）的文件
        unsigned int account;
        unsigned int password;
        vector<string>personalfile;//私人保存在服务器云端的文件
        bool islogin = false;
        vector<int>talkRooms;
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
        queue<string>remessage;//待接收消息队列
        queue<string>semessage;//待发送消息队列
        mutex semu;  //正在往发送消息队列里填消息
        mutex remu;  //接收消息的锁//服务线程用这两个队列的时候先被复制这两个互斥量然后加锁填数据如果没拿到锁会读到那看看之后优化成非阻塞模式可不可行
                     //拿到锁后还得再看一遍队列是否还有数据有可能之前的on_write已经读完了
        vector<string>mesage;//储存群内所有的消息
        vector<string>friend_queue;  //群内拥有成员数
        string name;//后面添加昵称/账号加好友功能//或者不整好友功能把所有服务器有的用户都发过去
        queue<string>filequ;//文件队列//待转发（0 file）或待接受（1 file）的文件
        unsigned int id;//群号
        unsigned int password;
        vector<string>file;//群文件
        int status = false; //群聊状态，存在，待删除
    };

extern unordered_map<int, queue<string>>handleingque;
extern mutex handle_acc_mutex;
    //extern shared_ptr<Server>Se_ptr;


//聊天室
extern int curMaxIndex;
extern unordered_map<int, vector<unsigned int>>talkRoomQueue;
extern mutex talkRoomQueue_mutex;

#endif