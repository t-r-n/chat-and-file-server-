#include"server.h"


bool is_have_task;
mutex my_mutex;
condition_variable semu_cond;

bool is_have_task1;
mutex my_mutex1;
condition_variable semu_cond1;


unordered_map<int, std::shared_ptr<clintchar>> account;  //用accountid索引的account集合
mutex acc_mutex;
unordered_map<int, std::shared_ptr<clint>> cur_account_ptr;
mutex cur_account_ptr_mutex;

unordered_map<int, bool>islogin;//在线用户指针速查
//客户端那边传文件应该另起一个线程不影响主线程通信


unordered_map<int, queue<string>>handleingque;
mutex handle_acc_mutex;


int curMaxIndex=10000;
unordered_map<int, vector<unsigned int>>talkRoomQueue;
mutex talkRoomQueue_mutex;

char* filePath;

Server::Server(char*path,int port) {
    filePath = path;

    sock_ptr sock1(new ip::tcp::socket(service));
    sock = sock1;
    boost::asio::io_service::work work(service);
    sock->open(ip::tcp::v4());
    acceptor = shared_ptr< ip::tcp::acceptor>(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)));
    cl.emplace_back(std::move(make_shared<clint>(ref(service))));
    cl.back()->this_it = cl.back();
    shared_ptr<clint>clptr = cl.back();
    clptr->id = curclintid++;
    error_code error;
    acceptor->async_accept(clptr->sock(),
        boost::bind(&Server::clint_handle_accept,
            this,
            boost::asio::placeholders::error,
            clptr));

    executor = make_shared< ilovers::TaskExecutor>(4);
    //executor->commit(&Server::handle_login_write, this);
    //executor->commit(&Server::translate, this);
    //executor->commit(std::bind(&Server::gc, this));
    executor->commit(std::bind(&Server::translate,this));//转发消息线程
    executor->commit(std::bind(&Server::handle_login_write, this));//在线用户接收消息线程
    executor->commit(std::bind(&Server::gc, this));//退出用户清楚内存线程
    cout << "ASYNC SERVER START" << endl;
    service.run();
    cout << "SERVER EXIT" << endl;
}
void Server::clint_handle_accept(boost::system::error_code er, shared_ptr<clint>cl1) {
    if (!er) {
        cl.emplace_back(std::move(make_shared<clint>(ref(service))));
        cl.back()->this_it = cl.back();
        shared_ptr<clint>clptr = cl.back();
        clptr->id = curclintid++;
        acceptor->async_accept(clptr->sock(),
            boost::bind(&Server::clint_handle_accept,
                this,
                boost::asio::placeholders::error,
                clptr));
        //#ifdef DEBUG
        cout << "客户端id：" << cl1->id << "已接入" << endl;
        //#endif // DEBUG
        cl1->on_read_plus();
    }else if (er != boost::asio::error::operation_aborted)
    {
        sock->close();
        std::cout << "acceptor error" << std::endl;
        exit(0);
    }
}
void Server::gc() {
    while (1) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
        unique_lock<mutex>gc_lock(cur_account_ptr_mutex, std::defer_lock);
        if (gc_lock.try_lock()) {
            for (auto it = cur_account_ptr.begin(); it != cur_account_ptr.end(); ) {
                if ((*it).second)
                    if ((*it).second->isdiascard) {
                        cout << "id:" << (*it).second->id << "已被清理" << endl;
                        cur_account_ptr.erase(it++);
                    }
                    else {
                        ++it;
                    }
                else {
                    ++it;
                }
            }
        }
        else {
        }
    }
}
void Server::changestatus(string& p, unsigned int st) {
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
void Server::translate() {
    static queue<string> tmpMesQueue;
    static Head h;
    while (1) {
        {
            lock_guard<mutex>acc_mutex_lock(acc_mutex);
            for (auto& a : account) {
                unique_lock<mutex>semu_lock(a.second->semu, std::defer_lock);
                if (semu_lock.try_lock()) {
                    if (a.second->semessage.size() > 0) {
                        tmpMesQueue.push( a.second->semessage.front() );
                        a.second->semessage.pop();
                    }
                    semu_lock.unlock();
                }
                else {
                }
            }
            int size = tmpMesQueue.size();
            while (!tmpMesQueue.empty()) {
                string s = tmpMesQueue.front();
                tmpMesQueue.pop();
                h = getHead(s);
                if (account.find(h.sendto) != account.end()) {
                    unique_lock<mutex>remu_lock(account[h.sendto]->remu,std::defer_lock);
                    if (remu_lock.try_lock()) {
                        account[h.sendto]->remessage.push(s);
                    }
                    else {//没拿到锁，放回消息队列
                        tmpMesQueue.push(s);
                    }
                }else {//不存在对方账户
                    cout << "未找到用户：" << h.sendto << endl;
                //写个状态返回消息
                }
                if ((--size) <= 0)break;
            }
        }

    }
    cout << "translate end" << endl;
}

void Server::handle_login_write() {//在线就把东西发过去
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//睡眠10毫秒
        unique_lock<mutex>ll(cur_account_ptr_mutex, std::defer_lock);
        if (ll.try_lock()) {
            //检查每一个在线用户需不需要调用write
            for (auto &a : cur_account_ptr) {
                if (cur_account_ptr[a.first] && !cur_account_ptr[a.first]->isdiascard) {//如果该指针不为空说明当前用户在线
                    unique_lock<mutex>lll(cur_account_ptr[a.first]->clch->remu, std::defer_lock);//如果接收队列可以访问   
                    if (lll.try_lock()) {
                        if (cur_account_ptr[a.first]->clch->remessage.size() > 0) {      
                            lll.unlock();//先解锁要不on_write那边没法取任务
                            cur_account_ptr[a.first]->on_write();//如果当前要发送的用户在线
                        }
                    }
                }
            }
        }
    }
}