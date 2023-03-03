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


            connection_manager connection_manager_;   //һ������ͻ����ӵ�����


            shared_ptr<boost::asio::io_context::work> work;
            int maxSIzeOnce = 1;//���ζ�ȡ�ļ�����ֽڣ�������ƽ��Ҫ������ļ��Ĵ�Сʱio������С���������
        };

    } // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP