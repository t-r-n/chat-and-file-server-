#ifndef CLINT_H
#define CLINT_H


#include "server_config.h"



class shared_const_buffer
{
public:
    // Construct from a std::string.
    explicit shared_const_buffer(const std::string& data)
        : data_(new std::vector<char>(data.begin(), data.end())),
        buffer_(boost::asio::buffer(*data_))
    {
    }

    // Implement the ConstBufferSequence requirements.
    typedef boost::asio::const_buffer value_type;
    typedef const boost::asio::const_buffer* const_iterator;
    const boost::asio::const_buffer* begin() const { return &buffer_; }
    const boost::asio::const_buffer* end() const { return &buffer_ + 1; }

private:
    boost::shared_ptr<std::vector<char> > data_;
    boost::asio::const_buffer buffer_;
};



struct clint :public std::enable_shared_from_this<clint> {
public:
    int tmpcount = 0;
    boost::asio::io_context::strand strand_;
    shared_ptr<clint> this_it;
    std::string inPath = "./fireRecv/*";
    Head* h;
    Head he;
    ip::tcp::socket sock_;
    std::string buf;
    std::string writebuf;
    std::string on_write1_buf;
    std::string name;
    std::string reme;
    int size;
    int id;
    bool islogout = false;
    bool isdiascard = false;
    char ttmphead[sizehead];
    int sizeofhead;
    shared_ptr<clintchar>clch;//只有在登陆后才会初始化
    std::list<std::string>reMessageList;
    mutex reMessageList_mu;


    clint(io_service& service) :sock_(service), strand_(service) {
        buf.resize(1024);
        sizeofhead = sizeof(he);
    }
    ip::tcp::socket& sock();
    void on_write_reback(int back,Head he=Head(),bool is=false);  //用于服务器向客户端发送各种状态码
    void on_write();
    void changestatus(std::string& p, unsigned int st);
    //void clint_handle_write(string p, boost::system::error_code er, size_t sz);
    //void clint_handle_write(std::list<std::string>::iterator it, boost::system::error_code er, size_t sz);
    //void clint_handle_write(shared_const_buffer buffer, boost::system::error_code er, size_t sz);
    void clint_handle_write( std::string,boost::system::error_code er, size_t sz);
    int headanylize(const Head* head);
    void on_read_content(Head he);
    //void on_read();
    void on_read_plus();
    void ls_file(std::string& s);
    ~clint() {
    }
    int errorhandle(boost::system::error_code& er);
    static Head getHead(std::string& buff) {
        Head hh;
        memcpy(&hh, buff.c_str(), sizehead);
        return hh;
    }
};

#endif // !CLINT_H