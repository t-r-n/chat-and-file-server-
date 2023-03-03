//#pragma once
#ifndef SERVER_H
#define SERVER_H
#include "server_config.h"
#include"clint.h"
//#include"FileServer.h"
class Server :public std::enable_shared_from_this<Server> {//服务器应该开一个线程池读取客户的消息队列调用相应客户的on_write函数
public:
    typedef shared_ptr<ip::tcp::socket> sock_ptr;
    typedef boost::system::error_code error_code;
    //单例模式:
    Server() = delete;
    Server(const Server& s) = delete;
    Server& operator=(const Server& s) = delete;
    static Server& getServer(char* path, int port = 8006) {
        static Server se(path,port);//再次调用不会重新声明
        return se;
     }
private:
    string buf;
    io_service service;
    sock_ptr sock;
    shared_ptr< ip::tcp::acceptor>acceptor;
    int curdfileid = 0; //描述如果当前加进来的是文件应该赋予的id
    int curclintid = 0;
    list<shared_ptr<clint>>cl;
    shared_ptr< ilovers::TaskExecutor>executor;

    Server(char*path,int port);
    void clint_handle_accept(boost::system::error_code er, shared_ptr<clint>cl1);
    void gc();//垃圾回收
    //开一个公共数据区开一个线程不断地读这个公共数据区如果该公共数据区有数据拿出来发送给相应的clint的消息接收容器
    //不对，如果是异步编程应该是去调用相应clintid的on_write方法
    char ttmphead[sizeof(Head)];
    Head* h;
    void changestatus(string& p, unsigned int st);
    static Head getHead(string& buff) {
        Head hh;
        memcpy(&hh, buff.c_str(),sizehead);
        return hh;
        //return *(fHead*)string(buff.begin(), buff.begin() + sizeofFhead).c_str();
    }
    //unordered_map<int, queue<string>>handleingque;
    //mutex handle_acc_mutex;
    void translate();
    void tmphandlethread();
    void handlequethread();
    void handle_login_write();
};

#endif