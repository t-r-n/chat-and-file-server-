#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include "connection.hpp"
#include "connection_manager.hpp"


namespace http {
    namespace server {

        /// The top-level class of the HTTP server.
        class server
        {
        public:
            server(const server&) = delete;
            server& operator=(const server&) = delete;


            explicit server(const std::string& address, const std::string& port,
                const std::string& doc_root, const int maxSizeofOnce);


            void run();

        private:

            void do_accept();


            void do_await_stop();

            boost::asio::io_context io_context_;


            boost::asio::signal_set signals_;

            boost::asio::ip::tcp::acceptor acceptor_;


            connection_manager connection_manager_;   //一个管理客户连接的容器


            shared_ptr<boost::asio::io_context::work> work;
            int maxSIzeOnce = 1;//单次读取文件最大字节，当大于平均要传输的文件的大小时io次数最小，网速最快
        };

    } // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP