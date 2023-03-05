#include "server.hpp"
#include <signal.h>
#include <utility>
namespace http {
    namespace server {

        server::server(const std::string& address, const std::string& port,const int maxSizeofOnce)
            : io_context_(1),
            signals_(io_context_),
            acceptor_(io_context_),
            connection_manager_(),
            maxSIzeOnce(maxSizeofOnce)
        {
            signals_.add(SIGINT);
            signals_.add(SIGTERM);
#if defined(SIGQUIT)
            signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

            do_await_stop();
            boost::asio::ip::tcp::resolver resolver(io_context_);
            boost::asio::ip::tcp::endpoint endpoint =
                *resolver.resolve(address, port).begin();
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen();
            work = make_shared< boost::asio::io_context::work>(io_context_);
            do_accept();
        }

        void server::run()
        {
            io_context_.run();
        }

        void server::do_accept()
        {
            acceptor_.async_accept(
                [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
                {
                    if (!acceptor_.is_open())
                    {
                        return;
                    }
                    if (!ec)
                    {
                        connection_manager_.start(std::make_shared<connection>( 
                            std::move(socket), connection_manager_,maxSIzeOnce));
                    }
                    do_accept();
                });
        }

        void server::do_await_stop()
        {
            signals_.async_wait(
                [this](boost::system::error_code /*ec*/, int /*signo*/)
                {
                    acceptor_.close();
                    connection_manager_.stop_all();
                });
        }

    }
}




