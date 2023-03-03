#ifndef CLINT_H
#define CLINT_H


#include "server_config.h"

struct clint :public std::enable_shared_from_this<clint> {
public:
    int tmpcount = 0;
    boost::asio::io_context::strand strand_;
    shared_ptr<clint> this_it;
    std::string inPath = "./fireRecv/*";
    Head* h;
    Head he;
    ip::tcp::socket sock_;
    string buf;
    string writebuf;
    string on_write1_buf;
    string name;
    string reme;
    int size;
    int id;
    bool islogout = false;
    bool isdiascard = false;
    char ttmphead[sizehead];
    int sizeofhead;
    shared_ptr<clintchar>clch;//只有在登陆后才会初始化

    clint(io_service& service) :sock_(service), strand_(service) {
        buf.resize(1024);
        sizeofhead = sizeof(he);
    }
    ip::tcp::socket& sock();
    void on_write_reback(int back,Head he=Head(),bool is=false);  //用于服务器向客户端发送各种状态码
    void on_write();
    void changestatus(string& p, unsigned int st);
    void clint_handle_write(string p, boost::system::error_code er, size_t sz);
    int headanylize(const Head* head);
    void on_read_content(Head he);
    void on_read();
    void on_read_plus();
    void ls_file(string& s);
    //void clint::on_read_http();
    ~clint() {
    }
    int errorhandle(boost::system::error_code& er);
    static Head getHead(string& buff) {
        Head hh;
        memcpy(&hh, buff.c_str(), sizehead);
        return hh;
    }
};

#endif // !CLINT_H