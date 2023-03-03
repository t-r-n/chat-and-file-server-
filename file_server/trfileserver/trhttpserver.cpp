#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "server.hpp"

int main(int argc, char* argv[])
{
    //std::cout << std::string().max_size() << std::endl;//最大限制2g那最大能传输的文件为2g？可我传了1g多就报alloc错误
    try
    {
        //127.0.0.1 8011 . / 1000
        http::server::server s(argv[1], argv[2], argv[3],atoi(argv[4]));
        s.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
    return 0;
}