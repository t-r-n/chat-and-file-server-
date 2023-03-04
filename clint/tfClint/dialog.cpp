#include "dialog.h"
#include "ui_dialog.h"
#include<QDebug>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;

}

void Dialog::on_pushButton_3_clicked()
{

    ww->show();
    *islogin=true;
    ww->getsock(sock_,islogin);
    this->hide();

}
bool Dialog::textistrue(string acc1,string pass1){
    try {
        stoi(acc1);
        stoi(pass1);
    } catch (...) {
        return 0;
    }
    return true;
}
void Dialog::on_pushButton_clicked()//注册响应槽
{
    acc=ui->lineEdit->text();
    pass=ui->lineEdit_2->text();
    bool ret=textistrue(acc.toStdString(),pass.toStdString());
    if(!ret) {
        QString s("账号或密码格式错误");
        ui->label->setText(s);
        return;
    }
    //账号密码格式正确
    he.type='i';
    he.length=0;
    he.account=acc.toUInt();
    he.mima=pass.toUInt();
    char ttmp[sizeof(Head)];
    memcpy(ttmp,&he,sizeof(Head));
    string buf(ttmp,sizeof(Head));
    sock_->write_some(buffer(buf));
    buf.clear();
    buf.resize(1024);
    sock_->read_some(buffer(buf));
    he=getHead(buf);
    //qDebug()<<he.status<<endl;
    if(he.status==reback::loginfal){
        QString s("注册失败");
        ui->label->setText(s);
        return;
    }
    QString s("注册成功");
    ui->label->setText(s);
}

void Dialog::on_pushButton_2_clicked()
{
    acc=ui->lineEdit->text();
    pass=ui->lineEdit_2->text();
    bool ret=textistrue(acc.toStdString(),pass.toStdString());
    if(!ret) {
        QString s("acc or pass false");
        ui->label->setText(s);
        return;
    }
    //账号密码格式正确
    he.type='l';
    he.length=0;
    he.account=acc.toUInt();
    he.mima=pass.toUInt();
    char ttmp[sizeof(Head)];
    memcpy(ttmp,&he,sizeof(Head));
    string buf(ttmp,sizeof(Head));
    sock_->write_some(buffer(buf));
    buf.clear();
    buf.resize(1024);
    unsigned int tmpacc=he.account;
    unsigned int tmppass=he.mima;
    sock_->read_some(buffer(buf));
    he=getHead(buf);
    //qDebug()<<he.status<<endl;
    if(he.status==reback::login){
        QString s("以在线");
        ui->label->setText(s);
        return;
    }
    else if(he.status==reback::loginfal){
        QString s("登陆失败");
        ui->label->setText(s);
        return;
    }
    else if(he.status==reback::logsucc){
        QString s("登陆成功");
        ui->label->setText(s);
        ww->acc=tmpacc;
        ww->pass=tmppass;
        ww->show();
        *islogin=true;
        ww->getsock(sock_,islogin);
        ww->init_start();
        this->hide();
        this->~Dialog();
        return;
    }
}
