//#pragma once
#ifndef SERVER_H
#define SERVER_H
#include "server_config.h"
#include"clint.h"
//#include"FileServer.h"
class Server :public std::enable_shared_from_this<Server> {//������Ӧ�ÿ�һ���̳߳ض�ȡ�ͻ�����Ϣ���е�����Ӧ�ͻ���on_write����
public:
    typedef shared_ptr<ip::tcp::socket> sock_ptr;
    typedef boost::system::error_code error_code;
    //����ģʽ:
    Server() = delete;
    Server(const Server& s) = delete;
    Server& operator=(const Server& s) = delete;
    static Server& getServer(char* path, int port = 8006) {
        static Server se(path,port);//�ٴε��ò�����������
        return se;
     }
private:
    string buf;
    io_service service;
    sock_ptr sock;
    shared_ptr< ip::tcp::acceptor>acceptor;
    int curdfileid = 0; //���������ǰ�ӽ��������ļ�Ӧ�ø����id
    int curclintid = 0;
    list<shared_ptr<clint>>cl;
    shared_ptr< ilovers::TaskExecutor>executor;

    Server(char*path,int port);
    void clint_handle_accept(boost::system::error_code er, shared_ptr<clint>cl1);
    void gc();//��������
    //��һ��������������һ���̲߳��ϵض������������������ù����������������ó������͸���Ӧ��clint����Ϣ��������
    //���ԣ�������첽���Ӧ����ȥ������Ӧclintid��on_write����
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