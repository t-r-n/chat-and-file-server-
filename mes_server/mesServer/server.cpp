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


unordered_map<int, queue<std::string>>handleingque;
mutex handle_acc_mutex;


int curMaxIndex=10000;
unordered_map<int, vector<unsigned int>>talkRoomQueue;
mutex talkRoomQueue_mutex;

char* filePath;
#ifdef COMYSQL
queue<std::string>sqlList;
mutex sqlList_mu;
#endif
Server::Server(char*path,int port) {
    filePath = path;

    /*��ʼ�����ݿ��ȡ���ݿ����ݻ���ע��͵�¼��ʱ���ȡ����*/
    //mysqlPool=
#ifdef COMYSQL
    unique_ptr<MysqlPool>mysqlptr(MysqlPool::getMysqlPoolObject());
    mysqlPool = std::move(mysqlptr);
    mysqlPool->setParameter("localhost", "root", "trn21123818", "mypro", 12345, NULL, 0, 2);
    initData();
#endif

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
    
    //executor->commit(std::bind(&Server::debugClintStatus, this));
    executor->commit(std::bind(&Server::translate,this));//ת����Ϣ�߳�
    executor->commit(std::bind(&Server::handle_login_write, this));//�����û�������Ϣ�߳�
    executor->commit(std::bind(&Server::gc, this));//�˳��û�����ڴ��߳�
#ifdef COMYSQL
    executor->commit(std::bind(&Server::handleSql, this));//�˳��û�����ڴ��߳�
#endif
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
        cout << "clint id��" << cl1->id << "enter" << endl;
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
    std::cout << "gc start" << std::endl;
    while (1) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
        unique_lock<mutex>gc_lock(cur_account_ptr_mutex, std::defer_lock);
        if (gc_lock.try_lock()) {
            for (auto it = cur_account_ptr.begin(); it != cur_account_ptr.end(); ) {
                if ((*it).second)
                    if ((*it).second->isdiascard) {
                        cout << "id:" << (*it).second->id << "discard" << endl;
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
void Server::changestatus(std::string& p, unsigned int st) {
    std::string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    h = (Head*)ttmphead;
    h->status = st;
    p = std::string(ttmphead, sizeof(Head)) + std::string(p.begin() + sizeof(Head), p.end());
}
void Server::translate() {//����ڴ�ۼ���remessage�����������û�
    std::cout << "translate start" << std::endl;
    static queue<std::string> tmpMesQueue;
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
                    else if(a.second->semessage.size()==0){//�ͷŷ��Ͷ����ڴ�
                        std::queue<std::string>().swap(a.second->semessage);
                    }
                    semu_lock.unlock();
                }
                else {
                }
            }
            int size = tmpMesQueue.size();
            while (!tmpMesQueue.empty()) {
                std::string s = tmpMesQueue.front();
                tmpMesQueue.pop();
                h = getHead(s);
                if (account.find(h.sendto) != account.end()) {
                    unique_lock<mutex>remu_lock(account[h.sendto]->remu,std::defer_lock);
                    if (remu_lock.try_lock()) {
                        account[h.sendto]->remessage.push(s);
                        //std::cout << "push remu:" << s << endl;
                    }
                    else {//û�õ������Ż���Ϣ����
                        tmpMesQueue.push(s);
                    }
                }else {//�����ڶԷ��˻�
                    cout << "no user��" << h.sendto << endl;
                //д��״̬������Ϣ
                }
                if ((--size) <= 0)break;
            }
        }

    }
    cout << "translate end" << endl;
}
void Server::debugClintStatus() {
    std::cout << "debugClintStatus start" << std::endl;
    while (1) {
        lock_guard<mutex>acc_mutex_lock(acc_mutex);
        for (auto& a : account) {
            unique_lock<mutex>semu_lock(a.second->semu, std::defer_lock);
            if (semu_lock.try_lock()) {
                std::cout << "acc:" << a.first << "seme size:"<<a.second->semessage.size() << std::endl;
                semu_lock.unlock();
            }
            unique_lock<mutex>remu_lock(a.second->remu, std::defer_lock);
            if (remu_lock.try_lock()) {
                std::cout << "acc:" << a.first << "reme size:" << a.second->remessage.size() << std::endl;
                remu_lock.unlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}
void Server::handle_login_write() {//���߾ͰѶ�������ȥ
    std::cout << "handle login write start" << std::endl;
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//˯��10����
        unique_lock<mutex>ll(cur_account_ptr_mutex, std::defer_lock);
        if (ll.try_lock()) {
            //���ÿһ�������û��費��Ҫ����write
            for (auto &a : cur_account_ptr) {
                if (cur_account_ptr[a.first] && !cur_account_ptr[a.first]->isdiascard) {//�����ָ�벻Ϊ��˵����ǰ�û�����
                    unique_lock<mutex>lll(cur_account_ptr[a.first]->clch->remu, std::defer_lock);//������ն��п��Է���   
                    if (lll.try_lock()) {
                        if (cur_account_ptr[a.first]->clch->remessage.size() > 0) {      
                            lll.unlock();//�Ƚ���Ҫ��on_write�Ǳ�û��ȡ����
                            cur_account_ptr[a.first]->on_write();//�����ǰҪ���͵��û�����
                        }
                    }
                }
            }
        }
    }
}
#ifdef COMYSQL
void Server::initData() {
    std::map<const std::string, std::vector<const char*> > m = mysqlPool->executeSql("select * from account");
    for (auto& a : m) {
        std::cout << a.first << ":" << " ";
        for (auto& b : a.second) {
            std::cout << b << " ";
        }
        std::cout << endl;
    }
    try {
        vector<int>accve;
        vector<int>pass;
        vector<vector<int>>rooms;
        for (std::map<const std::string, std::vector<const char*> >::iterator it = m.begin(); it != m.end(); ++it) {
            //std::cout << it->first <<" ";

            const std::string field = it->first;

            for (size_t i = 0; i < m[field].size(); i++) {
                //std::cout << m[field][i] << std::endl;  
                if (it->first == "acc") {
                    //std::cout << "acc:" << m[field][i] << endl;
                    int acc = std::stoi(m[field][i]);
                    accve.push_back(acc);

                }
                else if (it->first == "name") {
                    //std::cout <<"pass:" << m[field][i] << endl;
                    int pas = std::stoi(m[field][i]);
                    pass.push_back(pas);
                }
                else if (it->first == "rooms") {//Ⱥ��*Ⱥ��*Ⱥ��**  �� *
                    std::string text(m[field][i]);
                    //std::cout << "text:" << text << endl;
                    if (text[0] == '*') {
                        rooms.push_back(vector<int>());
                        continue;//���˻�δ��Ⱥ
                    }
                    vector<int>room;
                    int pos = 0;
                    for (int i = 0; i < text.size();) {
                        if ((pos=text.find("*")) != std::string::npos) {
                            int roomacc = std::stoi(text.substr(i, pos - i));
                            i = pos + 1;
                            room.push_back(roomacc);
                            if (i >= text.size() || text[i] == '*')break;//�ҵ���β
                        }
                        else {
                            break;
                        }
                    }
                    rooms.push_back(room);
                }
            }
        }
        std::cout << "size:" << accve.size() << " " << pass.size() << " " << rooms.size() << endl;
        if (accve.size() == pass.size() && accve.size() == rooms.size() ) {
            for (int i = 0; i < accve.size(); ++i) {
                account[accve[i]] = make_shared<clintchar>();
                account[accve[i]]->account = accve[i];
                account[accve[i]]->password = pass[i];
                account[accve[i]]->talkRooms = rooms[i];
            }
            for (auto& a : account) {
                for (int i = 0; i < a.second->talkRooms.size(); ++i) {
                    if (talkRoomQueue.find(a.second->talkRooms[i]) == talkRoomQueue.end()) {
                        talkRoomQueue[a.second->talkRooms[i]] = vector<unsigned int>();
                        talkRoomQueue[a.second->talkRooms[i]].push_back(a.first);
                    }
                    else {
                        talkRoomQueue[a.second->talkRooms[i]].push_back(a.first);
                    }
                }
            }
        }
        else {
            std::cout << "database data error" << std::endl;
            return;
        }
        std::cout << "init success" << std::endl;
    }
    catch (std::exception& e) {
        std::cout << e.what() << endl;
    }

}

void Server::handleSql() {
    while (1) {
        unique_lock<mutex>sql_lock(sqlList_mu, std::defer_lock);
        if (sql_lock.try_lock()) {
            while (!sqlList.empty()) {
                std::string tmp = sqlList.front();
                sqlList.pop();
                mysqlPool->executeSql(tmp.c_str());
            }
        }
    }
}
#endif