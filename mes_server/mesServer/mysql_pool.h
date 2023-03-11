#pragma once
#ifndef MYSQLPOOL_H
#define MYSQLPOOL_H

#include<iostream>
#include<mysql.h>
#include<queue>
#include<map>
#include<vector>
#include<utility>
#include<string>
#include<mutex>
#include<thread>

#pragma comment(lib,"libmysql.lib")
#pragma comment(lib,"wsock32.lib")
#include <mysqlx/xdevapi.h>
using namespace mysqlx;


class MysqlPool {
public:
    ~MysqlPool();
    std::map<const std::string, std::vector<const char* > > executeSql(const char* sql);//sql����ִ�к���
    static MysqlPool* getMysqlPoolObject();              //����ģʽ��ȡ����Ķ���
    void setParameter(const char* _mysqlhost,
        const char* _mysqluser,
        const char* _mysqlpwd,
        const char* _databasename,
        unsigned int  _port = 0,
        const char* _socket = NULL,
        unsigned long _client_flag = 0,
        unsigned int  MAX_CONNECT = 50);              //�������ݿ����
private:
    MysqlPool();
    MYSQL* createOneConnect();                    //����һ���µ����Ӷ���
    MYSQL* getOneConnect();                       //��ȡһ�����Ӷ���
    void close(MYSQL* conn);                      //�ر����Ӷ���
    bool isEmpty();                               //���ӳض��г��Ƿ�Ϊ��
    MYSQL* poolFront();                           //���ӳض��еĶ�ͷ
    unsigned int poolSize();                      //��ȡ���ӳصĴ�С
    void poolPop();                               //�������ӳض��еĶ�ͷ
private:
    std::queue<MYSQL*> mysqlpool;                 //���ӳض���
    const char* _mysqlhost;                     //mysql������ַ
    const char* _mysqluser;                     //mysql�û���
    const char* _mysqlpwd;                      //mysql����
    const char* _databasename;                  //Ҫʹ�õ�mysql���ݿ�����
    unsigned int  _port;                          //mysql�˿�
    const char* _socket;                        //�������ó�Socket or Pipeline��ͨ������ΪNULL
    unsigned long _client_flag;                   //����Ϊ0
    unsigned int  MAX_CONNECT;                    //ͬʱ����������Ӷ�������
    unsigned int  connect_count;                  //Ŀǰ���ӳص����Ӷ�������
    static std::mutex objectlock;                 //������
    static std::mutex poollock;                   //���ӳ���
    static MysqlPool* mysqlpool_object;           //��Ķ���
};

#endif