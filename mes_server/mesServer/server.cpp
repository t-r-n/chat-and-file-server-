#include"server.h"


bool is_have_task;
mutex my_mutex;
condition_variable semu_cond;

bool is_have_task1;
mutex my_mutex1;
condition_variable semu_cond1;


unordered_map<int, std::shared_ptr<clintchar>> account;  //��accountid������account����
mutex acc_mutex;
unordered_map<int, std::shared_ptr<clint>> cur_account_ptr;
mutex cur_account_ptr_mutex;

unordered_map<int, bool>islogin;//�����û�ָ���ٲ�
//�ͻ����Ǳߴ��ļ�Ӧ������һ���̲߳�Ӱ�����߳�ͨ��


unordered_map<int, queue<string>>handleingque;
mutex handle_acc_mutex;


int curMaxIndex=10000;
unordered_map<int, vector<unsigned int>>talkRoomQueue;
mutex talkRoomQueue_mutex;

Server::Server(int port) {
    sock_ptr sock1(new ip::tcp::socket(service));
    sock = sock1;
    boost::asio::io_service::work work(service);//����run�˳�
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
        cout << "�ͻ���id��" << cl1->id << "�ѽ���" << endl;
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
            cout << __LINE__ << "�õ���gc_lock" << endl;
#endif // DEBUG
            for (auto it = cur_account_ptr.begin(); it != cur_account_ptr.end(); ) {
                if ((*it).second)
                    if ((*it).second->isdiascard) {
                        cout << "id:" << (*it).second->id << "�ѱ�����" << endl;
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
            cout << __LINE__ << "�ò�������gc_lock" << endl;
#endif // DEBUG
        }
    }
}
void Server::changestatus(string& p, unsigned int st) {//��ߴ������п����ַ�������˵Ȼ���bug��������
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
                        cout << "catch lock" << endl;//ʲô��bug���Ӿͽ��ܲ�����Ϣ
                        tmpMesQueue.push( a.second->semessage.front() );
                        a.second->semessage.pop();
                    }
                    else {
                        //cout << "not catch lock1" << endl;//ʲô��bug���Ӿͽ��ܲ�����Ϣ
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
                    else {//û�õ������Ż���Ϣ����
                        tmpMesQueue.push(s);
#ifdef LOCK_DEBUG
                        cout << __LINE__ << "not catch lock:" << "remu_lock" << endl;
#endif
                    }
                }else {//�����ڶԷ��˻�
                    cout << "δ�ҵ��û���" << h.sendto << endl;
                //д��״̬������Ϣ
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
        semu_cond.wait(sbguard, [this]() {//Ϊ�õ�������˯�ߵȴ�������
            if (is_have_task == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "�õ���sb_guard" << endl;
#endif // DEBUG
        bool isCatchLock = true;
        for (auto& clchar : account) {//���������ü������ӳ�����

            string s;
            {//��semessage����
                //lock_guard<mutex>handle_lock((*it)->semu);
                std::unique_lock<std::mutex> handle_lock(clchar.second->semu, std::defer_lock);
                // print '*' if successfully locked, 'x' otherwise: 
#ifdef DEBUG
                cout << "�鿴�˻�" << clchar.first << "�Ĵ�ת����Ϣ" << endl;
#endif // DEBUG

                try {
                    if (handle_lock.try_lock()) {//�õ���//���û�õ�����߲�ֱ��˯���˰�
#ifdef DEBUG
                        cout << __LINE__ << "�õ���handle_lock" << endl;
#endif // DEBUG
                        if (clchar.second->semessage.size() > 0) {
                            s = clchar.second->semessage.front();
                            clchar.second->semessage.pop();
#ifdef DEBUG
                            cout << "��ȡ������" << s << endl;
#endif // DEBUG                
                        }
                    }
                    else {
                        //this_thread::yield();
                        isCatchLock = false;
                    }
                }
                catch (std::exception& e) {
                    cout << "�쳣��815�в���" << e.what() << endl;
                }
            }
            {
                if (s.size() >= sizeof(Head)) {
                    //std::unique_lock<std::mutex> handle_lock(handle_acc_mutex, std::defer_lock);
                    Head h = getHead(s);                   
                    lock_guard<mutex>han_Lock(handle_acc_mutex);
                    handleingque[h.sendto].push(s);
#ifdef DEBUG
                    cout << __LINE__ << "�õ���han_Lock" << endl;
#endif // DEBUG
                    is_have_task1 = true;
                    semu_cond1.notify_all();
                    //���⻽��ת���߳�//

                }
            }

        }
        if (isCatchLock)is_have_task = false;
        else is_have_task = true;//û�õ������ܻ�������Ҫ˯��
    }

}

void Server::handlequethread() {//�����ϣ������������߳�
    while (1) {
        unique_lock<mutex>sbguard1(my_mutex1);
        semu_cond1.wait(sbguard1, [this]() {
            if (is_have_task1 == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "�õ���sbguard1" << endl;
#endif // DEBUG
        //��������������˾�˯�ߣ������һ��forѭ��δ����������Ͳ�˯
        bool isCatchLock = true;
        {
            lock_guard<mutex>handleQueueLock(handle_acc_mutex);
#ifdef DEBUG
            cout << __LINE__ << "�õ���handleQueueLock" << endl;
#endif // DEBUG
            for (auto it = handleingque.begin(); it != handleingque.end(); ) {//�Ƿ�Ӧ������֮ǰ����?//Ҫ�ӣ�����������������Щ���⣬�ú�������ôд
                if (!it->second.empty()) {//������˺Ŵ�������Ϣ��Ϊ��
                    {
                        unique_lock<mutex>loo(acc_mutex, std::defer_lock);
                        if (loo.try_lock()) {
#ifdef DEBUG
                            cout << __LINE__ << "�õ���loo" << endl;
#endif // DEBUG
                            if (account.find(it->first) == account.end()) {
                                handleingque.erase(it++);
                                //--it;
                                continue;//���û�и��˺�ֱ�Ӽ���
                            }
                            unique_lock<mutex>lo(account[it->first]->remu, std::defer_lock);
                            if (lo.try_lock()) { //û�õ�����˯���õ����˾�˵���ܰѵ�ǰ����˴����꣬������ж��õ�����һ���ܴ�����
#ifdef DEBUG
                                cout << __LINE__ << "�õ���lo" << endl;
#endif // DEBUG
                                while (!it->second.empty()) {
                                    account[it->first]->remessage.push(it->second.front());
                                    it->second.pop();
                                }
                                it++;
                            }
                            else {
#ifdef DEBUG
                                cout << __LINE__ << "�ò�����lo" << endl;
#endif // DEBUG
                                isCatchLock = false;//��������δ����
                                it++;
                            }
                        }
                        else {
#ifdef DEBUG
                            cout << __LINE__ << "�ò�����lo" << endl;
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
void Server::handle_login_write() {//���߾ͰѶ�������ȥ
    while (1) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//˯��10����
        unique_lock<mutex>ll(cur_account_ptr_mutex, std::defer_lock);
        if (ll.try_lock()) {
#ifdef DEBUG
            cout << "�õ���ll" << endl;
#endif
            //���ÿһ�������û��費��Ҫ����write
            for (auto &a : cur_account_ptr) {
                if (cur_account_ptr[a.first] && !cur_account_ptr[a.first]->isdiascard) {//�����ָ�벻Ϊ��˵����ǰ�û�����
#ifdef DEBUG
                    cout << __LINE__ << "�û�" <<a.first<<"����" << endl;
#endif // DEBUG
                    unique_lock<mutex>lll(cur_account_ptr[a.first]->clch->remu, std::defer_lock);//������ն��п��Է���   
                    if (lll.try_lock()) {
#ifdef DEBUG
                        cout << __LINE__ << "�õ���lll" << endl;
#endif // DEBUG
                        if (cur_account_ptr[a.first]->clch->remessage.size() > 0) {      
                            lll.unlock();//�Ƚ���Ҫ��on_write�Ǳ�û��ȡ����
                            cur_account_ptr[a.first]->on_write();//�����ǰҪ���͵��û�����
                        }
                        else {
#ifdef DEBUG
                            cout << __LINE__ << "δ����" << a.first << "��on_write" << endl;
#endif // DEBUG

                        }
                    }
                    else {
#ifdef DEBUG
                        cout << __LINE__ << "�ò�����lll" << endl;
#endif // DEBUG
                    }
                }
            }
        }
        else {
#ifdef DEBUG
            cout << __LINE__ << "�ò�����ll" << endl;
#endif // DEBUG
        }
    }
    cout << "************************************************************88**�߳��˳�" << endl;
}