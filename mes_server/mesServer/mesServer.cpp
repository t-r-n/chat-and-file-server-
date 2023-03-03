#include <iostream>
#include "server.h"
int main()
{
    auto & se=Server::getServer();

    while (1);
    std::cout << "Hello World!\n";
}
