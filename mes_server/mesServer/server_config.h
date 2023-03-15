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
        passerror = 5,       //�������
        linkerror = 6,    //���ӶϿ���
        login = 7,        //�Ѿ�����
        logsucc = 8,    //��½�ɹ�
        loginsucc = 9,  //ע��ɹ�
        loginfal = 10,
        fitrfail = 11,  //�ļ�����ʧ��
        fitrbusy = 12,    //�ļ����з����������
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
#pragma pack(1)  //Ԥ����ʱ�ֽڲ�����
    struct Head {
        unsigned char type;//С�ļ�f�����packid��-1��-1�İ�������Ϣ����������ļ�����˵����һ�����ļ�����һ��file��������ָ�����vec��ÿ���മ����,id������������ɾ����λ��ָ����һ�������ù̶�ve����Ҳ����id��������һ���ļ���queue�ع������id
        unsigned int length;
        unsigned int id;//������ļ�����������ݴ�����Ӧ��id
        int          packid;//��ʱ���û��������ֶ� ��=��ͷ+�ı�+�û���
        unsigned int account;//�˺�
        unsigned int mima;//����
        unsigned int sendto;//0�������� ��0��Ҫ�������˻�
        unsigned int status = 0; //0��ʼ״̬(server�߳̿�ʼ����) 1//�Ѵ���(serverֱ������) //2//��Ϣ�ɶ���(serer������Ϣ����) packidΪ-1ʱ��ʾ�ļ���С
    };
#pragma pack()  //�ָ��ֽڶ���
    //const int sizeofhead = sizeof(Head);
    const int sizehead = sizeof(Head);
    struct clintchar {//ȫ��
        queue<std::string>remessage;//��������Ϣ����
        queue<std::string>semessage;//��������Ϣ����
        mutex semu;  //������Ϣ��
        mutex remu;  //������Ϣ����

        vector<int>friend_queue;//Ⱥ�Ź�������Ϣ�ÿͻ������Ⱥ�ž����﷢����Ϣ
        std::string name;//��������ǳ�/�˺żӺ��ѹ���//���߲������ѹ��ܰ����з������е��û�������ȥ
        queue<std::string>filequ;//�ļ�����//��ת����0 file��������ܣ�1 file�����ļ�

        unsigned int account;//�˺�
        unsigned int password;//����
        vector<std::string>personalfile;//˽�˱����ڷ������ƶ˵��ļ�
        bool islogin = false;//�Ƿ�����
        vector<int>talkRooms;//������
    };
    enum { MAXSIZE = 40 * 1024 + 64 };

    struct clint;
extern    unordered_map<int, std::shared_ptr<clintchar>> account;  //��accountid������account����
extern    mutex acc_mutex;
extern    unordered_map<int, std::shared_ptr<clint>> cur_account_ptr;
extern    mutex cur_account_ptr_mutex;


extern    unordered_map<int, bool>islogin;//�����û�ָ���ٲ�
    //�ͻ����Ǳߴ��ļ�Ӧ������һ���̲߳�Ӱ�����߳�ͨ��

    struct talkgroupchar {//Ⱥ����Ϣ //�ù���Ӧ�û�������Ϣͷ����뷢�Ͷ���Ҫ���͸���Ⱥ�飬clintchar��Ҫ�������е�Ⱥ���id
        queue<std::string>remessage;//��������Ϣ����
        queue<std::string>semessage;//��������Ϣ����
        mutex semu;  //������������Ϣ����������Ϣ
        mutex remu;  //������Ϣ����//�����߳������������е�ʱ���ȱ�����������������Ȼ��������������û�õ���������ǿ���֮���Ż��ɷ�����ģʽ�ɲ�����
                     //�õ����󻹵��ٿ�һ������Ƿ��������п���֮ǰ��on_write�Ѿ�������
        vector<std::string>mesage;//����Ⱥ�����е���Ϣ
        vector<std::string>friend_queue;  //Ⱥ��ӵ�г�Ա��
        std::string name;//��������ǳ�/�˺żӺ��ѹ���//���߲������ѹ��ܰ����з������е��û�������ȥ
        queue<std::string>filequ;//�ļ�����//��ת����0 file��������ܣ�1 file�����ļ�
        unsigned int id;//Ⱥ��
        unsigned int password;
        vector<std::string>file;//Ⱥ�ļ�
        int status = false; //Ⱥ��״̬�����ڣ���ɾ��
    };

extern unordered_map<int, queue<std::string>>handleingque;
extern mutex handle_acc_mutex;
    //extern shared_ptr<Server>Se_ptr;
#ifdef COMYSQL
extern queue<std::string>sqlList;
extern mutex sqlList_mu;
#endif


//������
extern int curMaxIndex;
extern unordered_map<int, vector<unsigned int>>talkRoomQueue;
extern mutex talkRoomQueue_mutex;


extern char* filePath;
#endif