#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
#define FILETR
#define _CRT_SECURE_NO_WARNINGS
#include <array>
#include <memory>
#include <boost/asio.hpp>
#ifdef FILETR
struct Fhead {
    char type;  //r,���ļ���s���ļ�
    int sendto;
    int content_length;  //�����δ��������ļ��͵�cur_offsetʹ��
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
#include"thread_pool.h"             //ֻ����Ӳ�ͬ�����;�̬��Ա����
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
            int ONCE_READ = 65536 * 16* 1;//100mb ,�ļ���ȡbufÿ���ܴ洢���������,
                            //�������һ�ζ�ȡ�ռ���ڴ���һ���ļ�������ʱtcp����
                            //��С����io����ÿ�ζ�ȡ65536�ֽ�
 
            //�ļ��ϴ����ط������ò���һ���˺�����
            int acc;
            int sendto;//0��ʾ���͸�������

            int content_length;  //�����ܳ���
            int file_length;// �ļ��ܳ���
            string filename;
            int sizeoffilename = 0;
            string headbuf;
            string contentbuf;
            int curFileSize = 0;  //��ǰ��ȡ�����ļ���С
            bool init = true;
            int cur_offset = 0;//����Ƕϵ������ļ����������ֵ
            ofstream outfile;
            ifstream infile;
            string path;//��¼�����ļ���·��
            //�����ٶȼ�¼����
            decltype(std::chrono::system_clock::now()) starttime; //auto�ھֲ�����������ʱ���ã������δ��ʼ������Ҫauto���ƵĹ���ʱ��
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