#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
#define FILETR
#define _CRT_SECURE_NO_WARNINGS
#include <array>
#include <memory>
#include <boost/asio.hpp>
#ifdef FILETR
struct Fhead {
    char type;  //r,收文件，s发文件
    int sendto;
    int content_length;  //如果是未传输完成文件就当cur_offset使用
    int acc;
    char filename[256];
    //int sizeoffilename;

};
const int sizeofFFhead(sizeof(Fhead));
const int BUFSIZE = 8192;
#include<fstream>
//#define DEBUG
#define WIN
//#define UNIX
#ifdef UNIX
#include <dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<string.h>
#endif
#include<stdlib.h>
//#define WIN 
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
#include"thread_pool.h"             //只能添加不同函数和静态成员函数
#include<fstream>
#include<boost/asio/strand.hpp>

using namespace std;
using namespace boost::asio;
#endif //FILETR
namespace http {
    namespace server {

        class connection_manager;


        class connection
            : public std::enable_shared_from_this<connection>
        {
        public:
            connection(const connection&) = delete;
            connection& operator=(const connection&) = delete;


            explicit connection(boost::asio::ip::tcp::socket socket,
                connection_manager& manager,int& maxSizeOnce);


            void start();

            void stop();

        private:

            void do_read();


            boost::asio::ip::tcp::socket socket_;


            connection_manager& connection_manager_;

            std::array<char, 8192> buffer_;


#ifdef FILETR
            int iocount = 0;
            //int maxSizeofOnce = 1;
            int ONCE_READ = 65536 * 16* 1;//100mb ,文件读取buf每次能存储的最大容量,
                            //当分配的一次读取空间大于传输一次文件的容量时tcp进行
                            //最小次数io，即每次读取65536字节
 
            //文件上传下载服务器用不上一下账号密码
            int acc;
            int sendto;//0表示发送给服务器

            int content_length;  //内容总长度
            int file_length;// 文件总长度
            string filename;
            int sizeoffilename = 0;
            string headbuf;
            string contentbuf;
            int curFileSize = 0;  //当前读取出的文件大小
            bool init = true;
            int cur_offset = 0;//如果是断点续传文件就设置这个值
            ofstream outfile;
            ifstream infile;
            string path;//记录读入文件的路径
            //下载速度记录部分
            decltype(std::chrono::system_clock::now()) starttime; //auto在局部变量声明的时候用，这个在未初始化但需要auto类似的功能时用
            string workPath = "./FileRecv/";

        public:
            std::atomic<bool> is_delete;
            void do_content_read();
            void do_content_write();
            //void delete_me();
            void ls_file();
            void do_FileRecv_write();
            //void error_handle();
#endif //FILETR

        };

        typedef std::shared_ptr<connection> connection_ptr;

    } // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP