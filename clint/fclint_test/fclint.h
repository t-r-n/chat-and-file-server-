#pragma once
#define UNIX_SERVER
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
//#include"thread_pool.h"             //ֻ����Ӳ�ͬ�����;�̬��Ա����
#include<fstream>
#include<boost/asio/strand.hpp>
#include"charconvert.h"

//#include<boost/thread.hpp>//������lib�ⷽ������-��������-������-������������E:\boost_1_79_0\boost_1_79_0\bin.v2\libs\thread\build\msvc-14.2\debug\address-model-32\link-static\threadapi-win32\threading-multi\libboost_thread-vc142-mt-gd-x32-1_79.lib
using namespace std;
using namespace boost::asio;
struct Fhead {
	char type;
	int sendto;
	int content_length;
	int acc;
	char filename[256];
	//int sizeoffilename;
};
const int sizeofFhead = sizeof(Fhead);

class fclint :public std::enable_shared_from_this<fclint> {
public:
	int acc;
	int sendto;
	fclint();
	fclint(char* path);
	void do_write();
	void do_read();
	void do_recv(string filename1);
	void do_content_recv();
	void ls_file();
	void stop();
private:
	int iocount = 0;
	ifstream infile;
	ofstream outfile;
	string content_buf;
	int content_length;
	decltype(std::chrono::system_clock::now()) starttime_read; //auto�ھֲ�����������ʱ���ã������δ��ʼ������Ҫauto���ƵĹ���ʱ��
	
	//��ÿ������
	decltype(std::chrono::system_clock::now()) starttime_seconds;
	int content_size = 0;
	
	std::shared_ptr<ip::tcp::socket>sock_;
	std::shared_ptr<ip::tcp::endpoint>ep;
	io_context service;
	string filename;
	string tmpbuf;
	bool init = true;
};
