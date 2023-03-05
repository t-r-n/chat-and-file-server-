#include "clint.h"
#include<QString>
#include<QDebug>
clint::clint(shared_ptr<ip::tcp::socket>sock)
{
    sock_=sock;
    re_buf.resize(1024);
    haveMes.store(false);
    wr_buf.resize(1024);

    vector<thread>ve;
    ve.push_back(thread(&clint::read_thread,this));
    ve.push_back(thread(&clint::write_thread,this));
    ve[0].detach();
    ve[1].detach();
}
void clint::read_thread(){
    for(;;){
        qDebug()<<"read_thread"<<endl;
        re_buf.clear();
        re_buf.resize(1024);
        sock_->read_some(buffer(re_buf));
        {
            lock_guard<mutex>re_lc(this->re_mutex);
            readqu.push(re_buf);
        }
        haveMes.store(true);
        re_mutex_cond.notify_all();
    }
}
void clint::write_thread(){
    bool isnew=false;
    for(;;){
        qDebug()<<"write_thread"<<endl;
        {
        unique_lock<mutex>un_lock(wr_mutex);
        wr_cond.wait(un_lock,[this](){
           if(is_have_task){
               return true;
           }
           return false;
        });
        }
        lock_guard<mutex>re_lc(this->wr_mutex);
        while(!writequ.empty()){
            if(writequ.size()>0){
                wr_buf=writequ.front();
                writequ.pop();
                isnew=true;
            }
            if(isnew){
                sock_->write_some(buffer(wr_buf));
                isnew=false;
            }
        }
        is_have_task=false;
    }
}
int clint::headanylize(Head &head){
    if (head.type == 'm') {
        return 1;
    }
    else if (head.type == 'f') {
        return 2;
    }else if(head.type=='r'){
        return 3;
    }else if(head.type == 'q'){
        return 4;
    }
    return 0;
}
