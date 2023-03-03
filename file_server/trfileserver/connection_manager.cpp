#include "connection_manager.hpp"

namespace http {
    namespace server {

        connection_manager::connection_manager()
        {
        }

        void connection_manager::start(connection_ptr c)
        {
            connections_.insert(c);
            c->start();//server那边调用connection的start传进来的参数c是Connection类的智能指针
        }

        void connection_manager::stop(connection_ptr c)
        {
            connections_.erase(c);
            c->stop();  
        }

        void connection_manager::stop_all()
        {
            for (auto c : connections_)
                c->stop();
            connections_.clear();
        }

    } // namespace server
} // namespace http