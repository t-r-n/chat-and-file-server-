// testServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<thread>
#include<string>
#include<vector>
#include<functional>
#include<memory>
#include<boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<string>
#include<condition_variable>
#include <ctime>
#include <random>

using namespace boost::asio;
using boost::system::error_code;
using namespace boost::asio;


using namespace std;
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
#pragma pack()  //预编译时字节不对齐

std::atomic<int>curConnCount=0;

std::string signUp(int acc,int pass) {
    Head he;
    he.type = 'i';
    he.length = 0;
    he.account = acc;
    he.mima = pass;
    char ttmp[sizeof(Head)];
    memcpy(ttmp, &he, sizeof(Head));
    return std::string (ttmp, sizeof(Head));

}

std::string signIn(int acc, int pass) {
    Head he;
    he.type = 'l';
    he.length = 0;
    he.account = acc;
    he.mima = pass;
    char ttmp[sizeof(Head)];
    memcpy(ttmp, &he, sizeof(Head));
    //string buf(ttmp, sizeof(Head));
    return std::string(ttmp, sizeof(Head));

}
std::string writeMessage(std::string mes,int acc,int sendto) {
    Head he;
    he.type = 'm';
    he.account = acc;
    he.sendto = sendto;
    he.length = mes.size();
    char ttmp[sizeof(Head)];
    memcpy(ttmp, &he, sizeof(Head));
    std::string package(ttmp, sizeof(Head));
    package += mes;
    return package;
}
void clintThread(int acc,int pass) {
    io_service service;
    shared_ptr<ip::tcp::socket>sock;
    shared_ptr<ip::tcp::endpoint>ep;
    ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("43.143.152.92"), 8006);

    sock = make_shared<ip::tcp::socket>(service);
  /*  boost::asio::ip::tcp::socket::bytes_readable cmd(true);
    sock->io_control(cmd);*/
    try
    {
        sock->connect(*ep);
    }
    catch (boost::system::system_error const& e)
    {
        std::cout << "Warning: could not connect : " << e.what() << endl;
    }
    string buf;
    buf.resize(1024);
    buf.clear();
    buf = signUp(acc, pass);
    sock->write_some(buffer(buf));
    buf.clear();
    buf.resize(1024);
    sock->read_some(buffer(buf));
    std::cout << "reback:" << buf << std::endl;
    buf.clear();
    buf.resize(1024);
    buf = signIn(acc, pass);
    sock->write_some(buffer(buf));
    buf.clear();
    buf.resize(1024);
    sock->read_some(buffer(buf));
    std::cout << "reback:" << buf << std::endl;
    buf.clear();
    buf.resize(1024);
    curConnCount++;
    while (1) {
        std::default_random_engine e;
        e.seed(time(0));
        //std::this_thread::get_id();
        buf = writeMessage("hello", acc,(e()%1000) +1200);
        sock->write_some(buffer(buf));
        buf.clear();
        buf.resize(1024);
        sock->read_some(buffer(buf));
        std::cout << buf << endl;
    }

}
int main()
{
    int offset = 1200;
    vector<std::thread>threads;
    for (int i = 0; i < 1000; ++i) {
        threads.push_back(std::thread(clintThread, i + offset, i + offset));
        threads.back().detach();
    }
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "curMaxConnect:" << curConnCount.load() << std::endl;
        //std::this_thread::yield();
    }
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
