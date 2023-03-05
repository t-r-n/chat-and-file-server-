#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "server.hpp"

int main(int argc, char* argv[])
{
   
    try
    {
        //127.0.0.1 8011  1000
        http::server::server s(argv[1], argv[2],atoi(argv[3]));
        s.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
    return 0;
}