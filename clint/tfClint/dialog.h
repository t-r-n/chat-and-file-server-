#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include"widget.h"
#include<QString>
namespace Ui {
class Dialog;
}
class Dialog : public QDialog
{
    Q_OBJECT

public:
    QString acc;
    QString pass;
    Head he;
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
    bool *islogin;
    Widget *ww;
    shared_ptr<ip::tcp::socket>sock_;
    void getlog(bool* islog,Widget*w,shared_ptr<ip::tcp::socket>sock){
        islogin=islog;
        ww=w;
        sock_=sock;
    }
    bool textistrue(string acc1,string pass1);
    void touchlabel(QString s){}; //信号有点问题不能正常传参
    Head getHead(string &buff) {
        //char tmphead[sizeof(Head)];
        string headme(buff.begin(), buff.begin() + sizeof(Head));
        //memcpy(tmphead, headme.c_str(), sizeof(Head));
        //Head h = *(Head*)ttmphead;
        //Head hh = *(Head*)ttmphead;
        Head hh;
        memcpy(&hh, headme.c_str(), sizeof(Head));
        return hh;
    }
private slots:
    void on_pushButton_3_clicked();


    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
