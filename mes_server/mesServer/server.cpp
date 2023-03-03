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

Server::Server(int port) {
    sock_ptr sock1(new ip::tcp::socket(service));
    sock = sock1;
    boost::asio::io_service::work work(service);//不让run退出
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
    //executor->commit(&Server::tmphandlethread, this);
    //executor->commit(&Server::handlequethread, this);
    //executor->commit(&Server::translate, this);
    executor->commit(std::bind(&Server::translate,this));
    executor->commit(std::bind(&Server::handle_login_write, this));
    executor->commit(std::bind(&Server::gc, this));
    cout << "ASYNC SERVER START" << endl;
    service.run();
    cout << "server exit" << endl;
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
#ifdef DEBUG
            cout << __LINE__ << "拿到锁gc_lock" << endl;
#endif // DEBUG
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
#ifdef DEBUG
            cout << __LINE__ << "拿不到到锁gc_lock" << endl;
#endif // DEBUG
        }
    }
}
void Server::changestatus(string& p, unsigned int st) {//这边处理完有可能字符串搞错了等会修bug考虑这里
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
void Server::translate() {
    static queue<string> tmpMesQueue;
    static Head h;
    cout << "translate start" << endl;
    while (1) {
        {
//#ifdef LOCK_DEBUG
//            cout <<__LINE__<<"catching lock:" << "acc_mute_lock" << endl;
//#endif
            lock_guard<mutex>acc_mutex_lock(acc_mutex);

            for (auto& a : account) {
                //cout << "acc:" << a.first << endl;
                unique_lock<mutex>semu_lock(a.second->semu, std::defer_lock);
                if (semu_lock.try_lock()) {
                    if (a.second->semessage.size() > 0) {
                        cout << "catch lock" << endl;//什么鬼bug不加就接受不了消息
                        tmpMesQueue.push( a.second->semessage.front() );
                        a.second->semessage.pop();
                    }
                    else {
                        //cout << "not catch lock1" << endl;//什么鬼bug不加就接受不了消息
                    }
                    semu_lock.unlock();
                }
                else {
                    #ifdef LOCK_DEBUG
                                cout <<__LINE__<<"not catching lock:" << "semu_lock" << endl;
                    #endif
                }
                
                //unique_lock<mutex>remu_lock()
            }
            int size = tmpMesQueue.size();
            if (size > 0)cout << "size:" << size << endl;
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
#ifdef LOCK_DEBUG
                        cout << __LINE__ << "not catch lock:" << "remu_lock" << endl;
#endif
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
#if 0
void Server::tmphandlethread() {
    while (1) {
        unique_lock<mutex>sbguard(my_mutex);
        semu_cond.wait(sbguard, [this]() {//为拿到锁阻塞睡眠等待被唤醒
            if (is_have_task == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "拿到锁sb_guard" << endl;
#endif // DEBUG
        bool isCatchLock = true;
        for (auto& clchar : account) {//这边起那面得加锁不加出问题

            string s;
            {//查semessage队列
                //lock_guard<mutex>handle_lock((*it)->semu);
                std::unique_lock<std::mutex> handle_lock(clchar.second->semu, std::defer_lock);
                // print '*' if successfully locked, 'x' otherwise: 
#ifdef DEBUG
                cout << "查看账户" << clchar.first << "的带转发消息" << endl;
#endif // DEBUG

                try {
                    if (handle_lock.try_lock()) {//拿到锁//如果没拿到锁这边不直接睡眠了啊
#ifdef DEBUG
                        cout << __LINE__ << "拿到锁handle_lock" << endl;
#endif // DEBUG
                        if (clchar.second->semessage.size() > 0) {
                            s = clchar.second->semessage.front();
                            clchar.second->semessage.pop();
#ifdef DEBUG
                            cout << "已取出任务" << s << endl;
#endif // DEBUG                
                        }
                    }
                    else {
                        //this_thread::yield();
                        isCatchLock = false;
                    }
                }
                catch (std::exception& e) {
                    cout << "异常被815行捕获" << e.what() << endl;
                }
            }
            {
                if (s.size() >= sizeof(Head)) {
                    //std::unique_lock<std::mutex> handle_lock(handle_acc_mutex, std::defer_lock);
                    Head h = getHead(s);                   
                    lock_guard<mutex>han_Lock(handle_acc_mutex);
                    handleingque[h.sendto].push(s);
#ifdef DEBUG
                    cout << __LINE__ << "拿到锁han_Lock" << endl;
#endif // DEBUG
                    is_have_task1 = true;
                    semu_cond1.notify_all();
                    //在这唤醒转发线程//

                }
            }

        }
        if (isCatchLock)is_have_task = false;
        else is_have_task = true;//没拿到锁可能还有任务不要睡眠
    }

}

void Server::handlequethread() {//处理哈希表中有任务的线程
    while (1) {
        unique_lock<mutex>sbguard1(my_mutex1);
        semu_cond1.wait(sbguard1, [this]() {
            if (is_have_task1 == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "拿到锁sbguard1" << endl;
#endif // DEBUG
        //如果把任务处理完了就睡眠，如果上一次for循环未把任务处理完就不睡
        bool isCatchLock = true;
        {
            lock_guard<mutex>handleQueueLock(handle_acc_mutex);
#ifdef DEBUG
            cout << __LINE__ << "拿到锁handleQueueLock" << endl;
#endif // DEBUG
            for (auto it = handleingque.begin(); it != handleingque.end(); ) {//是否应该在这之前加锁?//要加，这两个处理函数还有些问题，好好想想怎么写
                if (!it->second.empty()) {//如果该账号待处理消息不为空
                    {
                        unique_lock<mutex>loo(acc_mutex, std::defer_lock);
                        if (loo.try_lock()) {
#ifdef DEBUG
                            cout << __LINE__ << "拿到锁loo" << endl;
#endif // DEBUG
                            if (account.find(it->first) == account.end()) {
                                handleingque.erase(it++);
                                //--it;
                                continue;//如果没有该账号直接继续
                            }
                            unique_lock<mutex>lo(account[it->first]->remu, std::defer_lock);
                            if (lo.try_lock()) { //没拿到锁不睡，拿到锁了就说明能把当前这个人处理完，如果所有都拿到锁了一定能处理完
#ifdef DEBUG
                                cout << __LINE__ << "拿到锁lo" << endl;
#endif // DEBUG
                                while (!it->second.empty()) {
                                    account[it->first]->remessage.push(it->second.front());
                                    it->second.pop();
                                }
                                it++;
                            }
                            else {
#ifdef DEBUG
                                cout << __LINE__ << "拿不到锁lo" << endl;
#endif // DEBUG
                                isCatchLock = false;//还有任务未处理
                                it++;
                            }
                        }
                        else {
#ifdef DEBUG
                            cout << __LINE__ << "拿不到锁lo" << endl;
#endif // DEBUG
                            it++;
                        }
                    }
                }
                else {
                    it++;
                }
            }
        }
        if (isCatchLock) {
            is_have_task1 = false;
        }
        else {
            is_have_task1 = true;
        }
    }
}
#endif
void Server::handle_login_write() {//在线就把东西发过去
    while (1) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//睡眠10毫秒
        unique_lock<mutex>ll(cur_account_ptr_mutex, std::defer_lock);
        if (ll.try_lock()) {
#ifdef DEBUG
            cout << "拿到锁ll" << endl;
#endif
            //检查每一个在线用户需不需要调用write
            for (auto &a : cur_account_ptr) {
                if (cur_account_ptr[a.first] && !cur_account_ptr[a.first]->isdiascard) {//如果该指针不为空说明当前用户在线
#ifdef DEBUG
                    cout << __LINE__ << "用户" <<a.first<<"在线" << endl;
#endif // DEBUG
                    unique_lock<mutex>lll(cur_account_ptr[a.first]->clch->remu, std::defer_lock);//如果接收队列可以访问   
                    if (lll.try_lock()) {
#ifdef DEBUG
                        cout << __LINE__ << "拿到锁lll" << endl;
#endif // DEBUG
                        if (cur_account_ptr[a.first]->clch->remessage.size() > 0) {      
                            lll.unlock();//先解锁要不on_write那边没法取任务
                            cur_account_ptr[a.first]->on_write();//如果当前要发送的用户在线
                        }
                        else {
#ifdef DEBUG
                            cout << __LINE__ << "未调用" << a.first << "的on_write" << endl;
#endif // DEBUG

                        }
                    }
                    else {
#ifdef DEBUG
                        cout << __LINE__ << "拿不到锁lll" << endl;
#endif // DEBUG
                    }
                }
            }
        }
        else {
#ifdef DEBUG
            cout << __LINE__ << "拿不到锁ll" << endl;
#endif // DEBUG
        }
    }
    cout << "************************************************************88**线程退出" << endl;
}