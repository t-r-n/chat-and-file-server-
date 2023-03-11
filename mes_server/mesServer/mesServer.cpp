#include <iostream>
#include "server.h"
#include"mysql_pool.h"

int main(int argc,char**argv)
{
//命令行参数填写文件服务器的filerecv路径E:/trn_project/fin_server/file_server/trfileserver/FileRecv/*



    auto & se=Server::getServer(argv[1]);

    while (1);
    std::cout << "Hello World!\n";
}
