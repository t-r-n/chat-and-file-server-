
#include "widget.h"
#include "ui_widget.h"
#include<QDebug>
#include <synchapi.h>
#include <QMetaType>
#include <QTextCodec>
//#include<QTest>
#include<cctype>
#include<fstream>
#include<thread>
#include<chrono>
#include<cmath>
#include<time.h>
#include<QPalette>
#include<QPixmap>
Q_DECLARE_METATYPE(QString)

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{



    //QString::fromLocal8Bit ("中文");
    ui->setupUi(this);

    setAutoFillBackground(true);    // 这句要加上, 否则可能显示不出背景图.
    QPalette palette = this->palette();
       //palette.setColor(QPalette::Window, Qt::red);  // 设置背景色
       //palette.setBrush(this->backgroundRole(), Qt::black);// 设置背景色
    palette.setBrush(QPalette::Window,
               QBrush(QPixmap(":/image/VCG211304857146.webp").scaled(    // 缩放背景图.
                   this->size(),
                   Qt::IgnoreAspectRatio,
                   Qt::SmoothTransformation)));    // 使用平滑的缩放方式
    this->setPalette(palette);
    ui->textEdit_2->setStyleSheet("background-image:url(:/image/VCG211258127905.webp)");
    ui->textEdit->setStyleSheet("background-image:url(:/image/VCG211258127905.webp)");
    ui->lineEdit->setStyleSheet("background-image:url(:/image/VCG211258127905.webp)");
    ui->lineEdit_3->setStyleSheet("background-image:url(:/image/VCG211258127905.webp)");
    //qrc:/image/VCG211298027758.webp
    //this->setStyleSheet("border-image:url(:/image/VCG211298027758.webp)");
    //连接部分
    //ip::tcp::endpoint ep(ip::address::from_string(add.c_str()), portt);
//    ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8006);
//    sock_=make_shared<ip::tcp::socket>(service);
//    sock_->connect(ep);
    qRegisterMetaType<QString > ("QString");
    //void (Widget::*sig_mes)(QString,unsigned int)=&Widget::signal_mes;
    //connect(this,sig_mes,this,&Widget::signal_mes_solt);
    connect(this,SIGNAL(signal_mes(QString,unsigned int)),this,SLOT(signal_mes_solt(QString,unsigned int)));
    connect(this,SIGNAL(signal_per(QString)),this,SLOT(signal_per_solt(QString)));
    connect(this,&Widget::signal_mes_talk,this,[this](QString mes1,unsigned int from,int id){
        mes1=mes1+" --from"+QString::number(from);
        qDebug()<<"line:"<<__LINE__<<endl;
        //this->ui->lineEdit->setText(mes);  //textEdit_2
        stackWidget->setCurrentIndex(widgetId[id]);
        QString s=static_cast<QTextEdit*>(stackWidget->currentWidget())->toPlainText();
        s+=mes1;
        s+=" \n";
        qDebug()<<"line:"<<__LINE__<<endl;
        static_cast<QTextEdit*>(stackWidget->currentWidget())->setText(s);
        qDebug()<<"line:"<<__LINE__<<endl;
    });
    connect(this,&Widget::signal_file_recv,this,&Widget::signal_file_recv_slot);
//    exit_button=new QPushButton(this);
//    qDebug()<<this->x()<<" "<<this->y()<<endl;
//    qDebug()<<this->width()<<" "<<this->height()<<endl;
//    qDebug()<<exit_button->width()<<" "<<exit_button->y()<<endl;
//    exit_button->move(this->width()-(exit_button->width()/2),this->y());

    //ed=new QTextEdit(this);


    hLayout=new QHBoxLayout();
    stackWidget=new QStackedWidget(this);
    stackWidget->addWidget(ui->textEdit_2);
    stackWidget->resize(this->width()/2,this->height()*2/3);
//    QLabel*label1=new QLabel("窗口2",this);
//    stackWidget->addWidget(label1);

    listWidget=new QListWidget(this);
    listWidget->resize(this->width()/4,stackWidget->height());
    listWidget->addItem("默认对话框");
    //listWidget->addItem("窗口二");


    QHBoxLayout *widgetButton=new QHBoxLayout();
    widgetButton->addWidget(ui->pushButton);
    widgetButton->addWidget(ui->pushButton_6);
    vButtonLayout=new QVBoxLayout();

    QPushButton*BuildTalkRoomButton=new QPushButton("+",this);
    QPushButton*addTalkRoomButton=new QPushButton("++",this);
    vButtonLayout->addLayout(widgetButton);
    vButtonLayout->addWidget(BuildTalkRoomButton);
    vButtonLayout->addWidget(addTalkRoomButton);
    hLayout->addWidget(stackWidget,3);
    hLayout->addWidget(listWidget,1);
    hLayout->addLayout(vButtonLayout,1);
//    QLabel*label1=new QLabel("窗口2",this);
//    stackWidget->addWidget(label1);
//    listWidget->addItem("窗口二");



    QHBoxLayout*hModeLayout=new QHBoxLayout();
    hModeLayout->addWidget(ui->pushButton_4);
    hModeLayout->addWidget(ui->pushButton_5);
    hModeLayout->addWidget(ui->label);
    hModeLayout->addWidget(ui->lineEdit_3);
    hModeLayout->addWidget(ui->label_2);
    hModeLayout->addWidget(ui->label_3);
    hModeLayout->addWidget(ui->label_4);
    hModeLayout->addWidget(ui->lineEdit);

    QVBoxLayout*buttonGroup=new QVBoxLayout();
    buttonGroup->addWidget(ui->pushButton_2);
    buttonGroup->addWidget(ui->pushButton_3);
    QHBoxLayout*textGroup=new QHBoxLayout();
    textGroup->addWidget(ui->textEdit);
    textGroup->addLayout(buttonGroup);
    QVBoxLayout*allGroup=new QVBoxLayout(this);//layout只能有一个布局填this指针
    //allGroup->addLayout(widgetButton);
    allGroup->addLayout(hLayout);
    allGroup->addLayout(hModeLayout);
    allGroup->addLayout(textGroup);





    connect(addTalkRoomButton,&QPushButton::clicked,[=](){
        Head wr_he;//把注册登陆那边的缓冲区读写代码抄过来试试
        wr_he.type='d';
        wr_he.account=this->acc;
        wr_he.sendto=0;
        wr_he.length=0;
        addDialog add;
        if(add.exec()){
            wr_he.id=add.id().toInt();
        }
        char ttmphe[sizeof(Head)];
        memcpy(&ttmphe,&wr_he,sizeof(Head));
        string mes_buf=string(ttmphe,sizeof(Head));
        {
            lock_guard<mutex>wr_lock(cl->wr_mutex);
            cl->writequ.push(mes_buf);
            cl->is_have_task=true;
        }
        cl->wr_cond.notify_all();
    });
    connect(BuildTalkRoomButton,&QPushButton::clicked,[=](){//问题在服务器那边he.length=0后async_read未返回
        Head wr_he;//把注册登陆那边的缓冲区读写代码抄过来试试
        wr_he.type='r';
        wr_he.account=this->acc;
        wr_he.sendto=0;
        wr_he.length=0;
        char ttmphe[sizeof(Head)];
        memcpy(&ttmphe,&wr_he,sizeof(Head));
        string mes_buf=string(ttmphe,sizeof(Head));
        {
            lock_guard<mutex>wr_lock(cl->wr_mutex);
            cl->writequ.push(mes_buf);
            cl->is_have_task=true;
        }
        cl->wr_cond.notify_all();
    });
    connect(listWidget,&QListWidget::itemSelectionChanged,[=](){//设置当前窗口
        int tmp=listWidget->currentRow();
        stackWidget->setCurrentIndex(tmp);
        qDebug()<<"cur:"<<listWidget->currentItem()->text()<<endl;
    });
    connect(this,&Widget::buildTalkRooms,this,[this](int id){//注意lambda区别这个指定了信号发送者this是在widget下调用的lambda
        stackWidget->addWidget(new QTextEdit(this));
        listWidget->addItem(QString("%1").arg(id));
        widgetId[id]=curWidgetid++;//指定群号的实际窗口号的索引
    });
    connect(this,&Widget::signal_file,this,&Widget::signal_file_solt);
}
void Widget::signal_mes_solt(QString mes,unsigned int from){
//    for(int i=0;i<mes.size();++i){
//        qDebug()<<(int)mes[i].toLatin1()<<endl;
//    }
    mes=mes+" --from"+QString::number(from);
    //this->ui->lineEdit->setText(mes);  //textEdit_2

    QString s=ui->textEdit_2->toPlainText();
    //s+='\n';
    s+=mes;
    s+=" \n";

    this->ui->textEdit_2->setText(s);
}
void Widget::signal_per_solt(QString mes){
    QString s=ui->textEdit_2->toPlainText();
    s+=" \n";
    s+=mes;
    s+=" \n";
    this->ui->textEdit_2->setText(s);
}
void Widget::signal_file_recv_slot(QString buf){
    string s;
    for(auto &a:buf){
        s+=a.toLatin1();
    }
    Head he=getHead(s);
    s=string(s.begin()+sizeof(Head),s.begin()+(sizeof(Head)+he.length));
    QString ss=ui->textEdit_2->toPlainText();
    ss+=" \n";
    ss+=str2qstr(s);
    ss+=" from:";
    ss+=QString("%1").arg(he.account);
    ss+=" \n";
    this->ui->textEdit_2->setText(ss);
}
Widget::~Widget()
{
    delete ui;
}

void Widget::init_start(){ //启动线程
    cl=make_shared<clint>(sock_);
    //this->ui->textEdit_2->setText(s);
    //ui->label_3->setText(acc);
    this->ui->label_3->setText(QString(QString::number(acc)));
    //ui->
    //thread th(&Widget::handle_message,this);
    ve_th.push_back(std::thread(&Widget::handle_message,this));
    ve_th[0].detach();
}
void Widget::on_pushButton_clicked()
{
    this->close();
}

void Widget::on_pushButton_6_clicked()
{
    this->showMinimized();
}
int Widget::headanylize(Head &head){
    if (head.type == 'm') {
        return 1;
    }
    else if (head.type == 'f') {
        return 2;
    }else if(head.type=='r'){
        return 3;
    }else if(head.type=='d'){
        return 4;
    }else if(head.type=='q'){
        return 5;
    }else if(head.type=='s'){
        return 6;
    }else if(head.type=='t'){
        return 7;
    }
    return 0;
}
void Widget::handle_message(){
    //string han_str;
    Head re_he;
    for (;;) {
        string han_str;
        unique_lock<mutex>trlock(cl->re_mutex);
        cl->re_mutex_cond.wait(trlock,[this](){
            if(this->cl->haveMes.load()){
                cl->haveMes.store(false);//尽早置false防止读线程又读到消息重置该变量
                return true;
            }
            return false;
        });
        if(cl->readqu.size()>0){
            han_str=cl->readqu.front();
            cl->readqu.pop();
        }else{
            continue;
        }
        trlock.unlock();
//        if(trlock.try_lock()){
//            if(cl->readqu.size()>0){
//                han_str=cl->readqu.front();
//                cl->readqu.pop();
//            }else{
//                std::this_thread::yield();
//                continue;
//            }
//            trlock.unlock();
//        }else{
//            std::this_thread::yield();
//            continue;
//        }
        re_he=getHead(han_str);
        int ret=headanylize(re_he);
        //Head he=getHead(han_str);
        //qDebug()<<"talkhe->type:"<<he.type<<"he->acc"<<he.account<<"he->sen:"<<he.sendto<<"he.id:"<<he.id<<endl;
        if(ret==1){
            string mes(han_str.begin()+sizeof(Head),han_str.begin()+(sizeof(Head)+re_he.length));
            int from=re_he.account;
            QByteArray bitarray(mes.c_str(),mes.length());
            QString sstmp=bitarray;
            qDebug()<<"接收回的数据:"<<sstmp<<endl;
            emit this->signal_mes(sstmp,from);
            //emit this->signal_mes(QString::fromStdString(mes),from);
            //            //string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
        }else if(ret==2){
            QString s;
            for(int i=0;i<han_str.size()&&i<(sizeofhead+re_he.length);++i){
                qDebug()<<han_str[i];
                s+=han_str[i];
            }
            emit signal_file_recv(s);

            han_str.clear();//清空字符串否则一直进入这个循环
        }else if(ret==3){//创建群聊加入群聊成功返回群聊id
            emit buildTalkRooms(re_he.id);
            qDebug()<<"发送信号"<<endl;
            qDebug()<<"接收到的服务器的回馈"<<re_he.type<<" length:"<<re_he.length<<" status:"<<re_he.status<<"id:"<<re_he.id<<endl;
//            if(re_he.status<=0){//说明是请求到的文件id
//                {
//                    lock_guard<mutex>lockk(fireid_mu);
//                    fireid.push(re_he.status);
//                }
//            }

        }else if(ret==4){
            string mes1(han_str.begin()+sizeof(Head),han_str.begin()+(sizeof(Head)+re_he.length));
            //QString mes(str2qstr(mes1));
            int from=re_he.account;
            emit this->signal_mes(QString::fromStdString(mes1),from);

        }
        else if(ret==5){
            string mes1(han_str.begin()+sizeof(Head),han_str.begin()+(sizeof(Head)+re_he.length));
            QString mes(str2qstr(mes1));
            QString status;
            if(re_he.status==1){
                status="receiving";
            }else if(re_he.status==2){
                status="received";
            }else if(re_he.status==3){
                status="receiv error";
            }else{
                status="未知错误";
            }
            mes+=" status: "+status;
            //mes+=" status: "+std::to_string(re_he.status);
            int from=re_he.account;
            emit this->signal_mes(mes,from);
        //emit this->signal_mes(QString::fromStdString(mes),from);
        //            //string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));

        }else if(ret==6){
            string mes1(han_str.begin()+sizeof(Head),han_str.begin()+(sizeof(Head)+re_he.length));
            QString mes(str2qstr(mes1));
            QString status;
            if(re_he.status==1){
                status="receiving";
            }else if(re_he.status==2){
                status="received";
            }else if(re_he.status==3){
                status="receiv error";
            }else{
                status="未知错误";
            }
            mes+=" status: "+status;
            //mes+=" status: "+std::to_string(re_he.status);
            int from=re_he.account;
            emit this->signal_mes(mes,from);
        //emit this->signal_mes(QString::fromStdString(mes),from);
        //            //string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));

        }else if(ret==7){//群聊消息

            string mes(han_str.begin()+sizeof(Head),han_str.begin()+(sizeof(Head)+re_he.length));
            int from=re_he.account;


            QByteArray bitarray(mes.c_str(),mes.length());
            QString sstmp=bitarray;//string到QString的无损转换防止中文乱码


            emit this->signal_mes_talk(sstmp,from,re_he.id);


            //emit this->signal_mes(QString::fromStdString(mes),from);
            //            //string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));

        }
        han_str.clear();
    };
}

void Widget::on_pushButton_2_clicked()
{


//----主窗口聊天：
    QString s=ui->textEdit->toPlainText();
//    int chnum=0;
//    for(auto a:s){
//        if(isalpha(a.toLatin1())){
//        }
//        else ++chnum;
//    }
//    s+=QString(chnum,' ');//还要加和字符一样长度的空格才能结局问题几个中文就需要几个空格
    if(s.size()>500){
        ui->textEdit->setText("字数超过限制");
        return;
    }

    string stmp=s.toStdString();
//    QByteArray bitarray(stmp.c_str(),stmp.length());
//    QString sstmp=bitarray;//解决QString和string互转中文乱码问题




    Head wr_he;
    wr_he.type='m';
    wr_he.account=this->acc;
    wr_he.sendto=ui->lineEdit_3->text().toInt();
    wr_he.length=stmp.size();
    if(stackWidget->currentIndex()>0){//聊天室消息
        //type :"t" id:群号 //acc发送者
        wr_he.type='t';
        wr_he.id=listWidget->currentItem()->text().toInt();

    }
    char ttmphe[sizeof(Head)];
    memcpy(&ttmphe,&wr_he,sizeof(Head));
    string mes_buf=string(ttmphe,sizeof(Head))+stmp;
    {
        lock_guard<mutex>wr_lock(cl->wr_mutex);
        cl->writequ.push(mes_buf);
        cl->is_have_task=true;
    }
    cl->wr_cond.notify_all();
}
void Widget::lifilethread(string path,string name){
    //const int READ_BUFF_SIZ=10*1024;
    io_service servicef;
    shared_ptr<ip::tcp::socket>sockf;
    shared_ptr<ip::tcp::endpoint>ep;
    try {
        ep=make_shared<ip::tcp::endpoint>(ip::address::from_string(ipstr.c_str()), 8007);
        qDebug()<<"文件线程要连入的ip"<<ipstr.c_str()<<endl;
        sockf=make_shared<ip::tcp::socket>(servicef);
        sockf->connect(*ep);
    } catch (...) {
        //ui->textEdit->setText("link 195 error");
        qDebug()<<__LINE__<<" error"<<endl;
        return;
    }


    int sen;
    long pretellg=0;
    Head he;
    fstream ff;
    string buf = path + name;
    int length;
    try {
            ff.open(buf.c_str(), ios::in | ios::binary);
            ff.seekg(0, ff.end);   //追溯到流的尾部
            length = ff.tellg();  //获取流的长度
            ff.seekg(0, ff.beg);  //回到流的头部
            sen=ui->lineEdit_3->text().toInt();
    }
    catch (...) {
            cout<< __LINE__ << " error" << endl;
            return;
    }

        he.type = 'f';
        he.length = name.size();
        he.packid = -1;
        he.id = 0;
        he.status = length;
        string st((char*)&he, sizeof(Head));
        st += name;
        try {
            write(*sockf, buffer(st, st.size()));
        }
        catch (...) {
            cout << __LINE__ << " error" << endl;
            return;
        }
        int packid = 0;
        int ret;

        //读文件缓冲区
        const int READ_BUFF_SIZ = 40 * 1024;
        //char buff[READ_BUFF_SIZ];
        bool isfalse = false;

        buff = new char[READ_BUFF_SIZ];


        //下载速度记录部分
        std::chrono::system_clock::time_point now;//= chrono::system_clock::now();
        std::chrono::system_clock::time_point now1;
        std::chrono::nanoseconds d; //= now.time_since_epoch();
        std::chrono::nanoseconds d1;
        now1 = std::chrono::system_clock::now();
        d1 = now1.time_since_epoch();
        std::chrono::nanoseconds origind;
        origind = d1;
        int increaseofminute = 0;


        //const int sizehead=sizeof(Head);
        fHead fHe;
        st.resize(READ_BUFF_SIZ +48);//发送缓冲区
        bool tmp = false;
        while (1) {

            //下载速度记录
            now = std::chrono::system_clock::now();
            d = now.time_since_epoch();
            double dd = ((d - d1).count() * pow(10, -9));
            if (dd > 1) {
                d1 = d;
                cout<<"当前下载速度为"<< (increaseofminute / dd) / 1024.0 / 1024.0<<"mb/s" << endl;
                increaseofminute = 0;
            }



            //cout<<"开始readsome"<<endl;
            try {
                //read(*sock, buffer(st,st.size()),transfer_exactly(29));
                sockf->read_some(buffer(st, st.size()));
            }
            catch (...) {
                //ui->textEdit->setText("256 error");
                cout<< __LINE__ << " error" << endl;
                return;
            }
            //cout<<"读到东西"<<endl;
            he = getHead(st);
            if (he.status == reback::fitrfail) {
                isfalse = true;
                break;
            }else if (he.status == reback::firetrst) { //cout<< "接收到firetrst目前packid" << packid << endl;
                pretellg = ff.tellg();
                //cout <<"对方已接收字节数：" << pretellg << endl;
                //packid++;
            }
            else if(he.status == reback::readerror){
                //try {
                //    size_t size = write(*sock, buffer(st, st.size()), transfer_all());
                //}
                //catch (...) {
                //    cout << __LINE__ << " error" << endl;
                //    return;
                //}
                //continue;
                tmp = true;
                cout << "prebackerror" << endl;
                --packid;
                ff.seekg(pretellg);
            }
            //pretellg = ff.tellg();
            try {
                ff.read(buff, READ_BUFF_SIZ);
            }
            catch (...) {
                //ui->textEdit->setText("249 error");
                cout<< __LINE__ << " error" << endl;
                return;
            }
            ret = ff.gcount();
            increaseofminute += ret;
            //qDebug()<<"写入"<<ret<<"个字节"<<endl;
            if (ret == 0)break;
            fHe.type = 'f';
            fHe.length = ret;
            fHe.packid = packid++;
            fHe.status = 1;
            if (ret < READ_BUFF_SIZ) {
                fHe.status = 2;
            }
            try {
                //st=string(tmp,sizeof(Head));
                st = string((char*)&fHe, sizeofFhead) + string(buff, 40960);
                string stmp(st.begin(), st.begin() + sizeofFhead);

                if (tmp) {
                    fHead* ftmp = (fHead*)stmp.data();
                    cout <<"重新向对方发送包" << ftmp->packid << "  " << ftmp->length << "  " << ftmp->type << endl;
                }
            }
            catch (std::exception& e) {

                cout << __LINE__ << " error" << endl;
                return;
            }
            try {
                //fHead he = getfHead(st);
                //cout<< "he.packid: " << he.packid << " he.id " << he.id << " he.statue " << he.status << endl;

                //size_t size=sockf->write_some(buffer(st));
                size_t size = write(*sockf, buffer(st, st.size()), transfer_all());
                //cout<< "写入的长度: " << size << endl;
                //cout<<st<<endl;
                //qDebug() << QString::fromStdString(st) << endl;
            }
            catch (...) {
                //ui->textEdit->setText("267 error");
                cout<< __LINE__ << " error" << endl;
                return;
            }
            //cout<<"写入"<<ret<<"个字节"<<endl;
        }
        if (ff.bad()) {
            cout << "文件发送失败fail" << endl;
            //ui->textEdit->setText("305 error");
            cout<< __LINE__ << " error" << endl;
            return;
        }
        if (isfalse) {
            cout<< "文件发送失败fail" << endl;
            //ui->textEdit->setText("310 error");
            cout<< __LINE__ << " error" << endl;
            return;
        }
        cout<< "文件发送成功success" << endl;



        try {//下载速度记录
            now = std::chrono::system_clock::now();
            d = now.time_since_epoch();
            double dd = ((d - origind).count() * pow(10, -9));
            int timesudu = 0;
            if (dd <= 0)return;
            timesudu = (length / dd) / 1024.0 / 1024.0;
            cout << "下载速度为" << timesudu << "mb/s" << endl;
            QString speed = QString("速度为%1 mb/s").arg(timesudu);
            QString s;
            s += QString::fromStdString(name) + " 发送成功 " + speed;
            signal_per(s);
        }
        catch (...) {
            return;
        }

    //发送向对方发送文件的消息
    Head wr_he;
    wr_he.type='f';
    wr_he.account=this->acc;
    wr_he.sendto=sen;
    wr_he.length=name.size();
    wr_he.status=1;//一是发送，2是发送成功的反馈，3是发送失败的反馈
//    char ttmphe[sizeof(Head)];
//    memcpy(&ttmphe,&wr_he,sizeof(Head));
    string mes_buf=string((char*)&wr_he,sizeof(Head))+name;
    {
        lock_guard<mutex>wr_lock(cl->wr_mutex);
        cl->writequ.push(mes_buf);
        cl->is_have_task=true;
    }
    cl->wr_cond.notify_all();

    ff.close();


}

void Widget::refilethread(string path,string name){
    const int BUFSIZE=40*1024;
    io_service servicef;
    shared_ptr<ip::tcp::socket>sockf;
    shared_ptr<ip::tcp::endpoint>ep;
    ep=make_shared<ip::tcp::endpoint>(ip::address::from_string(ipstr.c_str()), 8007);
    qDebug()<<"文件线程要连入的ip"<<ipstr.c_str()<<endl;
    sockf=make_shared<ip::tcp::socket>(servicef);
    sockf->connect(*ep);

    Head he;
    fHead fhe;
    fstream ff;
    string buf=path+name;
    ff.open(buf.c_str(),ios::out|ios::binary|ios::trunc);


    int sen=ui->lineEdit_3->text().toInt();
    he.type='y';
    he.length=name.size();
    he.status=1;
    buf=string((char*)&he,sizeof(Head));
    buf+=name;
    try {
        sockf->write_some(buffer(buf));
        //write(*sockf,buffer(buf,buf.size()),transfer_all());
    } catch (...) {
        qDebug()<<"387 write error"<<endl;


        rebackfalse(sen,name);


        return;
    }

    qDebug()<<"已向对方发布初始要接受文件的信息,等待接收文件大小"<<endl;

    try{
        sockf->read_some(buffer(buf));
        //read(*sockf,buffer(buf),transfer_exactly(40981));
    }catch(...){
        qDebug()<< __LINE__ <<" write error"<<endl;
        rebackfalse(sen,name);
        return;
    }
    try {
        he=getHead(buf);
    } catch (...) {
        qDebug()<< __LINE__ <<" write error"<<endl;
        rebackfalse(sen,name);
        return;
    }
    //he=getHead((buf));
    int filelength=he.length;
    int sizeofhead=sizeof(Head);
    int cursize=0;
    bool trfalse=false;
    qDebug()<<"待接收文件大小"<<filelength<<endl;
    he.status=reback::firetrst;
    string a((char*)&he,sizeof(Head));
    string s;
    try {
        s.resize(BUFSIZE+1024);
    } catch (std::exception &e) {
        qDebug()<<QString::fromStdString(e.what())<<endl;
        qDebug()<< __LINE__ <<" write error"<<endl;
        rebackfalse(sen,name);

        return;
    }
    //s.resize(7000);


    std::chrono::system_clock::time_point now ;//= chrono::system_clock::now();
    std::chrono::system_clock::time_point now1;
    std::chrono::nanoseconds d; //= now.time_since_epoch();
    std::chrono::nanoseconds d1;
    now1=std::chrono::system_clock::now();
    d1=now1.time_since_epoch();

    buf.clear();
    buf.resize(BUFSIZE+1024);
    while(1){
        //qDebug()<<"";

        try{
            sockf->write_some(buffer(a));
        }catch(...){
            //ui->textEdit->setText("357 error");
            qDebug()<< __LINE__ <<" write error"<<endl;
            rebackfalse(sen,name);
            return;
        }
        try{
            int res=read(*sockf,buffer(buf,buf.size()),transfer_exactly(40981));
            //cout<<"receive for"<<res<<"bytes"<<endl;
        }catch(boost::system::system_error e){
            qDebug()<< __LINE__ <<" write error"<<endl;
            qDebug()<<e.what()<<endl;
            cout<<e.code()<<endl;
            rebackfalse(sen,name);
            return;
        }

        try{
            fhe=getfHead(buf);
        }catch(...){
            //ui->textEdit->setText("357 error");
            qDebug()<< __LINE__ <<" write error"<<endl;
            rebackfalse(sen,name);
            return;
        }
        try{
            s=string(buf.begin()+sizeofFhead,buf.begin()+(sizeofFhead+fhe.length));
        }catch(...){
            int i;
            for(i=sizeofhead;i<buf.size();++i){
                if(buf[i]=='\0')break;
            }
            qDebug()<<"接收错误时接收到的数据量"<<i<<endl;
           Head *h=(Head*)(string(buf.begin(),buf.begin()+sizeof(Head)).c_str());
           qDebug()<<"h->length:"<<h->length<<endl;
           qDebug()<<"buf"<<buf.size()<<"   he.length:"<<he.length<<"内容："<<QString::fromStdString(buf)<<endl;
           qDebug()<< __LINE__ <<" write error"<<endl;


           rebackfalse(sen,name);


           return;
        }
        //qDebug()<<"读取到"<<he.length<<"字节"<<endl;
        cursize += fhe.length;
        //qDebug()<<"cursize:"<<cursize<<endl;
        //cout<<"writing int file "<<fhe.length<<"bytes"<<endl;
        if (ff.is_open()) {
            ff.write(s.c_str(),fhe.length);
            //cout<<"fire is open"<<endl;
        }
        else {
            trfalse = true;
            qDebug()<< __LINE__ <<" write error"<<endl;
        }
        if (fhe.status == 2) {//最后一个包
            if (cursize == filelength) {//接收完毕
                ff.close();
                fhe.status=reback::logsucc;
                a=string((char*)&he,sizeof(Head));

                break;
            }
            else {
                trfalse = true;
                qDebug()<<__LINE__<<"error"<<endl;
            }
        }
        if (ff.bad()) {
            trfalse = true;
            qDebug()<<__LINE__<<"error"<<endl;
            ff.close();
            qDebug()<<"文件传输失败"<<endl;


            //发送向对方发送文件的消息
            rebackfalse(sen,name);

            return;
        }
        if (trfalse) {//出问题了on_write error然后直接关闭
            ff.close();
            qDebug()<<"文件传输失败"<<endl;

            qDebug()<< __LINE__ <<" write error"<<endl;
            //发送向对方发送文件的消息
            rebackfalse(sen,name);


            return;
        }
    }
    qDebug()<<"文件传输成功"<<endl;
    sockf->close();

    try {
        now = std::chrono::system_clock::now();
        d = now.time_since_epoch();
        double dd = ((d - d1).count() * pow(10, -9));
        int timesudu=0;
        if (dd <= 0)return;
        timesudu = (filelength / dd) / 1024.0 / 1024.0;
        qDebug()<< "下载速度为" << timesudu << "mb/s" << endl;
        QString speed=QString("速度为%1 mb/s").arg(timesudu);
        QString s;
        s+=QString::fromStdString(name)+" 接收成功 "+speed;
//        QString s=ui->textEdit_2->toPlainText();
//        s+=" \n";
//        s+=QString::fromStdString(name)+" 下载成功 "+speed;
//        s+=" \n";
//        this->ui->textEdit_2->setText(s);
        signal_per(s);
    }
    catch (...) {
        return;
    }



    //发送向对方发送文件的消息
    Head wr_he;
    wr_he.type='f';
    wr_he.account=this->acc;
    wr_he.sendto=sen;
    wr_he.length=name.size();
    wr_he.status=2;//一是发送，2是发送成功的反馈，3是发送失败的反馈
//    char ttmphe[sizeof(Head)];
//    memcpy(&ttmphe,&wr_he,sizeof(Head));
    string mes_buf=string((char*)&wr_he,sizeof(Head))+name;
    {
        lock_guard<mutex>wr_lock(cl->wr_mutex);
        cl->writequ.push(mes_buf);
        cl->is_have_task=true;
    }
    cl->wr_cond.notify_all();

}

void Widget::rebackfalse(int sen,string name){
    Head wr_he;
    wr_he.type='f';
    wr_he.account=this->acc;
    wr_he.sendto=sen;
    wr_he.length=name.size();
    wr_he.status=3;//一是发送，2是发送成功的反馈，3是发送失败的反馈
    string mes_buf=string((char*)&wr_he,sizeof(Head))+name;
    {
        lock_guard<mutex>wr_lock(cl->wr_mutex);
        cl->writequ.push(mes_buf);
        cl->is_have_task=true;
    }
    cl->wr_cond.notify_all();

}
void Widget::on_pushButton_5_clicked()
{
    //system("./")
    int sen=ui->lineEdit_3->text().toInt();
    string name=ui->lineEdit->text().toStdString();


    Head he;
    he.account=this->acc;
    he.sendto=sen;
    he.type='f';
    he.status=1;//接收文件
    char tmp[sizeofhead];
    memcpy(&tmp,&he,sizeofhead);
    string stmp(tmp,sizeofhead);
    //QString she(QString::fromStdString(stmp));

    //QString she(tmp);



//    qDebug()<<"tmp->acc "<<((Head*)tmp)->account<<"tmp->sen:"<<((Head*)tmp)->sendto<<endl;
//    string s(she.toLatin1().data(),sizeofhead);
//    Head h=getHead(s);
//    qDebug()<<"hh->acc:"<<h.account<<"h->sen:"<<h.sendto<<endl;

    std::thread([=](){
        //string s("C:/Users/trn123456/Desktop/build-tfClint-Desktop_Qt_5_14_2_MinGW_32_bit-Release/release/fclint_test.exe s ");
        string s("./fclint_test.exe s ");
        s+=name;
        s+=" ";
        s+=to_string(sen);
        s+=" ";
        s+=to_string(acc);



        int ret=system(s.c_str());
        if(ret==1){
            Head h=getHead(stmp);
                qDebug()<<"hh->acc:"<<h.account<<"h->sen:"<<h.sendto<<endl;
                QByteArray bitarray(stmp.c_str(),sizeofhead);
                QString she;
                for(int i=0;i<stmp.size();++i){
                    she+=stmp[i];
                }
                //QString she=bitarray;//string到QString的无损转换防止中文乱码
//                string s(she.toLatin1().data(),sizeofhead);
//                Head he=getHead(s);
//                qDebug()<<"he->acc:"<<he.account<<"h->sen:"<<he.sendto<<endl;
//                string ss=she.toStdString();
//                Head h=getHead(ss);
//                    qDebug()<<"hh->acc:"<<h.account<<"h->sen:"<<h.sendto<<endl;
            //QString she(QString::fromStdString(stmp));


            emit signal_file(QString::fromStdString(name),1,1,she);
        }
    }).detach();
}
void Widget::signal_file_solt(QString name,int re_se,int isSucc,QString she){
    //发送成功通知对方
    //name=name.mid(name.indexOf('/'));
    for(int i=name.size()-1;i>=0;--i){
        if(name[i]=='/'){
            name=name.mid(i+1);
        }
    }

    string s(she.toLatin1().data(),sizeofhead);
    //qDebug()<<"s.size:"<<s.size()<<endl;
    //string s(bitarray.data(),bitarray.size());
    Head he=getHead(s);
    //qDebug()<<"he->acc:"<<he.account<<"h->sen:"<<he.sendto<<endl;
    if(re_se){//如果是发送文件成功（成功发送到服务器）
        qDebug()<<__LINE__<<endl;
        he.length=name.size();
        char ttmphe[sizeof(Head)];
        memcpy(&ttmphe,&he,sizeof(Head));
            //QString s=QString("%1将").arg(he.account)+"文件发送给:"+QString("%1").arg(he.sendto);
//            QString s=this->ui->textEdit->toPlainText();
//            s+=QString("%1将").arg(he.account)+"文件发送给:"+QString("%1").arg(he.sendto);
//            this->ui->textEdit->setText(s);
        string mes_buf=string(ttmphe,sizeof(Head))+qstr2str(name);
        {
            lock_guard<mutex>wr_lock(cl->wr_mutex);
            cl->writequ.push(mes_buf);
            cl->is_have_task=true;
        }
        cl->wr_cond.notify_all();
    }else{//如果文件接收成功

    }
}
void Widget::on_pushButton_4_clicked()
{
    int sen=ui->lineEdit_3->text().toInt();
    string name=ui->lineEdit->text().toStdString();
    std::thread([this,name,sen](){
        string s("C:/Users/trn123456/Desktop/build-tfClint-static-Release/release/fclint_test.exe r ");
        s+=name;
        s+=" ";
        s+=to_string(sen);
        s+=" ";
        s+=to_string(acc);

        system(s.c_str());
    }).detach();
#if 0
    string name=ui->lineEdit->text().toStdString();
    //string path="C:/Users/trn123456/Desktop/build-tfClint-Desktop_Qt_5_14_2_MinGW_64_bit-Debug/debug/";
    string path="./";
    //std::thread th(&Widget::lifilethread,this,path,name);
    std::thread th(&Widget::refilethread,this,path,name);
    th.detach();
#endif
}

void Widget::on_pushButton_3_clicked()
{
    ui->textEdit->setText("");
}

void Widget::on_pushButton_7_clicked()
{
    //发送向对方发送文件的消息
    Head wr_he;
    wr_he.type='d';
    wr_he.account=this->acc;
    wr_he.sendto=this->acc;
    wr_he.length=0;
    string mes_buf=string((char*)&wr_he,sizeof(Head));
    {
        lock_guard<mutex>wr_lock(cl->wr_mutex);
        cl->writequ.push(mes_buf);
        cl->is_have_task=true;
    }
    cl->wr_cond.notify_all();

}
//void Widget::mousePressEvent(QMouseEvent *event)
//{
//    // 如果是鼠标左键按下
//    if(event->button() == Qt::LeftButton)
//   {
//        this->move(this->x()+event->x(),this->y()+event->y());
//    }
//    // 如果是鼠标右键按下
////    else if(event->button() == Qt::RightButton)
////   {
////       ···
////    }
//}



void Widget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        mousePress = true;
    }
    //窗口移动距离
    movePoint = event->globalPos() - pos();
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    mousePress = false;
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if(mousePress)
    {
        QPoint movePos = event->globalPos();
        move(movePos - movePoint);
    }
}


