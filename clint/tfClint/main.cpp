#include "widget.h"
#include "dialog.h"
#include <QApplication>
#include<QString>
#include <QtCore/QTextCodec>
#include<memory>
#include<QDebug>
io_service service;
shared_ptr<ip::tcp::socket>sock;
bool islogin=false;

int main(int argc, char *argv[])
{
    cout<<"start"<<endl;
    string ipstr;
    shared_ptr<ip::tcp::endpoint>ep;
    //连接部分
    //ip::tcp::endpoint ep(ip::address::from_string(add.c_str()), portt);
    if(argc>=2){//连接服务器IP地址
        ep=make_shared<ip::tcp::endpoint>(ip::address::from_string(argv[1]), 8006);
        ipstr=string(argv[1]);
        //ip::tcp::endpoint ep(ip::address::from_string(argv[1]), 8006);
    }
    else{
        //ep=make_shared<ip::tcp::endpoint>(ip::address::from_string("192.168.1.4"), 8006);

        ep=make_shared<ip::tcp::endpoint>(ip::address::from_string("127.0.0.1"), 8006);
        ipstr=string("127.0.0.1");
        //ep=make_shared<ip::tcp::endpoint>(ip::address::from_string("43.143.152.92"), 8006);
        //ipstr=string("43.143.152.92");

    }

    sock=make_shared<ip::tcp::socket>(service);

    try
    {
        sock->connect(*ep);
    }
    catch (boost::system::system_error const& e)
    {
        qDebug() << "Warning: could not connect : " << e.what() << endl;
    }
    QApplication a(argc, argv);
    Dialog d;

    d.show();

    Widget w;
    w.ipstr=ipstr;
    d.getlog(&islogin,&w,sock);
    //w.setWindowFlags(Qt::FramelessWindowHint);
    w.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    return a.exec();
}
