#ifndef WIDGET_H
#define WIDGET_H
#include"clint.h"
#include<boost/asio.hpp>
#include<boost/bind.hpp>
#include <QWidget>
#include<QPushButton>
#include<QLabel>
#include<QLineEdit>
#include<thread>
#include <QMouseEvent>
#include<QStackedWidget>
#include<QListWidget>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QTextEdit>
#include<unordered_map>
#include"adddialog.h"
#include<QButtonGroup>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    string ipstr;
    io_service service;
    shared_ptr<ip::tcp::socket>sock_;
    vector<std::thread>ve_th;
    unsigned int acc;
    unsigned int pass;
    char* buff;  //开辟在堆区的缓冲区声明
    void getsock(shared_ptr<ip::tcp::socket>sock,bool *islog){
        sock_=sock;
        islogin=*islog;
    }
    void init_start();
    Head getHead(const string &buff) {
        //char tmphead[sizeof(Head)];
        string headme(buff.begin(), buff.begin() + sizeof(Head));
        Head hh;
        memcpy(&hh, headme.c_str(), sizeof(Head));
        return hh;

    }

    static inline fHead getfHead(string &buff) {
        return *(fHead*)string(buff.begin(), buff.begin() + sizeofFhead).c_str();
    }
    int headanylize(Head &head);
    void handle_message();
    QString str2qstr(const string str)  //用这两个函数支持带中文的QString和string互转
    {
        return QString::fromLocal8Bit(str.data());
    }

    string qstr2str(const QString qstr)
    {
        QByteArray cdata = qstr.toLocal8Bit();
        return string(cdata);
    }
    bool mousePress=false;
    QPoint  movePoint;
    void lifilethread(string path,string name);
    void refilethread(string path,string name);
    void rebackfalse(int sen,string name);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
signals:
    void signal_mes(QString,unsigned int);
    void signal_mes_talk(QString,unsigned int,int);
    void signal_per(QString);
    void signal_file(QString,int,int,QString);//1：文件名2：接收/发送3：成功/失败，head
    void buildTalkRooms(int id);
    void signal_file_recv(QString);
private slots:

    void on_pushButton_clicked();
    void signal_file_recv_slot(QString);
    void on_pushButton_6_clicked();
    void signal_mes_solt(QString mes,unsigned int from);
    void signal_per_solt(QString mes);
    void signal_file_solt(QString,int,int,QString);
    void on_pushButton_2_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_7_clicked();

//    void on_pushButton_8_clicked();

private:
    shared_ptr<clint>cl;
    QPushButton *exit_button;
    Ui::Widget *ui;
    bool islogin=false;
    queue<int>fireid;
    mutex fireid_mu;

    QTextEdit *ed;
    QStackedWidget* stackWidget;
    QListWidget* listWidget;
    std::unordered_map<int,int>widgetId;//群号-窗口索引号
    int curWidgetid=1;
    QHBoxLayout *hLayout;
    QVBoxLayout*vButtonLayout;
};
#endif // WIDGET_H
