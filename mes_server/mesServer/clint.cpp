#include"clint.h"

void clint::on_write_reback(int back,Head he,bool is) {
    if (isdiascard) {
        return;
    }
    if (!is) {
        he.type = 'r';//服务器消息回馈
        he.status = back;
        he.length = 0;
    }
    memcpy(ttmphead, &he, sizeof(Head));
    on_write1_buf = string(ttmphead, sizeof(Head));
    async_write(sock_, buffer(on_write1_buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
        //async_write(sock_, buffer(buf1), [=, this](boost::system::error_code er, size_t size) {
        if (er) {
            errorhandle(er);
            return;
        }
    });
}
void clint::on_read_content(Head he) {
    if (isdiascard) {
        return;
    }
    buf.clear();  //好像是这边导致的断言问题
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    
    async_read(sock_, buffer(buf), transfer_exactly(he.length), strand_.wrap([this, self,he](boost::system::error_code er, size_t sz) {
        if (er) {
            errorhandle(er);
            return;
        }
        int ret = headanylize((const Head*)&he);
        h = (Head*)&he;
        std::cout <<"message:" << buf << endl;
        switch (ret) {
            case 1: {//消息转发
           
                buf.insert(0, string((char*)&he, sizehead));
                {
                    if (clch) {
#ifdef LOCK_DEBUG
                        cout << __LINE__ << "catching lock:" << "selock" << endl;
#endif
                        lock_guard<mutex> selock(clch->semu);

                        clch->semessage.push(buf);
                        on_write_reback(reback::sendSucc);//发送成功通知
                        is_have_task = true;
                        semu_cond.notify_all();//唤醒处理任务的线程
                    }
                    else {
                        std::cout << "un login maybe attack" << std::endl;
                        //lock_guard<mutex> selock(acc_mutex);
                        //if (account.find(he.account) != account.end()) {
                        //    lock_guard<mutex> seelock(account[he.account]->semu);
                        //    account[he.account]->semessage.push(buf);
                        //}
                    }
                }
                break;
            }
            case 2: {
                string tmpbuf(buf.data(),h->length);
                
                buf = tmpbuf;
                ls_file(buf);
                ((Head*)&he)->length = buf.size();
                buf.insert(0, string((char*)&he, sizehead));
                {
                    if (clch) {
                        lock_guard<mutex> selock(clch->semu);
                        clch->semessage.push(buf);
                        is_have_task = true;
                        semu_cond.notify_all();
                    }
                }
                break;
            }
            case 3: {//登陆验证
                cout << __LINE__ << endl;
                unsigned int tmppassword = 0;
                {
                    lock_guard<mutex>acc_lock(acc_mutex);
                    if (account.find(h->account) != account.end()) {//有该账户
                        tmppassword = account[h->account]->password;
                    }
                    else {
                        on_write_reback(reback::loginfal);//登陆失败
                        //self->on_read();
                        self->on_read_content(Head());
                        return;
                    }
                }
                if (tmppassword != h->mima) {//密码不正确;
                    this->on_write_reback(reback::passerror);
                }
                else {//登陆成功//把全局的clint拷贝一份 

                    bool isloogin = false;
                    {
                        lock_guard<mutex>cur_log_lock(cur_account_ptr_mutex);
#ifdef DEBUG
                        cout << __LINE__ << "拿到锁cur_log_lock" << endl;
#endif // DEBUG
                        if (cur_account_ptr.find(h->account) == cur_account_ptr.end()) {
                            cur_account_ptr[h->account] = this_it;
                        }
                        else {
                            if (cur_account_ptr[h->account]) {
                                if (cur_account_ptr[h->account]->isdiascard) {
                                    cur_account_ptr[h->account] = this_it;
                                }
                                else {
                                    isloogin = true;
                                }
                            }
                            else {
                                cur_account_ptr[h->account] = this_it;
                            }
                        }
                    }
                    if (isloogin) {
                        on_write_reback(reback::login);
                    }
                    else {
                        clch = account[h->account];  
                        on_write_reback(reback::logsucc);
                        cout << "login succ" << endl;

                    }
                }
                break;
            }
            case 4: {

                clch = make_shared<clintchar>();
                bool issuc = false;
                {
                    lock_guard<mutex>login_lock(acc_mutex);
                    if (account.find(h->account) == account.end()) {
                        account[h->account] = clch;
                        issuc = true;
                    }
                    else {
                        on_write_reback(reback::loginfal);
                        cout << "sign up error" << endl;
                    }
                }
                if (issuc) {
#ifdef COMYSQL
                    //添加sql语句
                    string tmp="insert into account (acc,pass) values (";
                    tmp += std::to_string(h->account);
                    tmp += ',';
                    tmp+= std::to_string(h->mima);
                    tmp += ')';
                    
                    {
                        lock_guard<mutex>sqlLock(sqlList_mu);
                        sqlList.push(tmp);
                    }
#endif

                    clch->account = h->account;
                    clch->password = h->mima;
                    on_write_reback(reback::loginsucc);
                    cout << "sign up succ" << endl;
                }
                break;
            }
            case 5: {
                cout << __LINE__ << endl; //遍历该群聊向群聊里拥有的成员压入semessage
                break;
            }
            case 6: {//加入群聊
                {
                    
                    lock_guard<mutex>talkRoomLock(talkRoomQueue_mutex);
                    if (talkRoomQueue.find(h->id) == talkRoomQueue.end()) {//没找到该群号
                        on_write_reback(reback::readerror);
                        break;
                    }
                    talkRoomQueue[h->id].push_back(h->account );
                    if (clch) {
                        clch->talkRooms.push_back(h->id);
                    }
                    Head he;
                    he.type = 'r';
                    he.length = 0;
                    he.id = h->id;
                    on_write_reback(reback::BuildTalkRoomSuc, he, true);//返回群聊创建成功的消息
                }
                break;
            }
            case 7: {//创建群聊
                {
                    lock_guard<mutex>talkRoomLock(talkRoomQueue_mutex);
                    talkRoomQueue[curMaxIndex++] = { h->account };
                    if (clch) {
                        clch->talkRooms.push_back(curMaxIndex - 1);
                    }
                    Head he;
                    he.type = 'r';
                    he.length = 0;
                    he.id = curMaxIndex - 1;
                    on_write_reback(reback::BuildTalkRoomSuc, he, true);//返回群聊创建成功的消息
                }
                break;
            }
            case 8: {
                cout << __LINE__ << endl;
                break;
            }
            case 9: {
                
                buf.insert(0, string((char*)&he, sizehead));
                {
                    lock_guard<mutex>talkRoomQueue_lock(talkRoomQueue_mutex);
                    for (int i = 0; i < talkRoomQueue[he.id].size(); ++i) {
                        {
                            int tmp = talkRoomQueue[he.id][i];
                            //cout << "正在往" << tmp << "里压入群消息" << endl;
                            lock_guard<mutex>account_lock(acc_mutex);
                            lock_guard<mutex>semu_lock(account[talkRoomQueue[he.id][i]]->remu);
                            account[talkRoomQueue[he.id][i]]->remessage.push(buf);
                        }
                    }

                }
                break;
            }
            default: {
                cout << __LINE__ << endl;
                break;
            }
        }
        self->on_read_plus();
    }));
}
void clint::on_read_plus() {
    if (isdiascard) {
        return;
    }
    buf.clear(); 
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    async_read(sock_, buffer(buf), transfer_exactly(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        if (er) {
            errorhandle(er);
            return;
        }
        Head h = getHead(buf);
        int ret = headanylize((const Head*)buf.c_str());
        std::cout << hex << buf << endl;
        std::cout << "h.type:" << h.type << "h.acc:" << h.account << "h.sen:" << h.sendto << "h.length:" << h.length << std::endl;
        if ((h.length >= 0&&h.length<1024)&&ret!=0) {
            self->on_read_content(h);
            return;
        }
        else {
            //std::cout << "这里应该清空socket缓冲区" << std::endl;
            
            if (clch) {//如果是在线用户清除内存再关闭连接
                {
                    isdiascard = true;
                    islogout = true;
                    sock_.close();
                }
            }
            else {//断开连接
                {
                    sock_.close();
                }
            }
            return;
            self->on_read_plus();
            
        }
    }));
}
#if 0
void clint::on_write() {
    if (isdiascard) {
        return;
    }
    reme.clear();
    reme.resize(1024);
    {
        lock_guard<mutex> relock(clch->remu);

        if (clch->remessage.size() > 0&& clch->remessage.size()<10000) {//这边想想怎么搞一次性全发********如果这里一下子积压太多消息一条一条发就太慢了
            //限制一个账户的消息队列//如果短时间超过10000就认为该账户被攻击了//清空接收队列，停止转发后面的消息//也就是一个账户

            reme = clch->remessage.front();
            //将消息状态置为已处理
            clch->remessage.pop();
        }
        else {
            std::queue<std::string>().swap(clch->remessage);//为空释放掉申请的内存************************
            return;//多加一层认证防止各种不确定性
        }
    }
    
    //cout << "clint.cpp" << __LINE__ << "send one mes" << endl;
    async_write(sock_, buffer(reme, reme.size()), transfer_at_least(reme.size() - 1), bind(&clint::clint_handle_write, this, reme, _1, _2));//这边占位符可能还有些问题
}
void clint::clint_handle_write(string p, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        errorhandle(er);
        //出错将消息重新置为初始态压入队列
        changestatus(p, 0);
        {
            lock_guard<mutex>relock(clch->remu);
            clch->remessage.push(p);
        }
        return;
    }
    static int count = 0;
    std::cout << "write one message" << count++ << endl;
    //将消息状态置为可丢弃压入队列
    this->on_write();
}
#endif


void clint::on_write() {
    if (isdiascard) {
        return;
    }
    std::queue<std::string> tmpQueue;
    {
        lock_guard<mutex> relock(clch->remu);
        if (!clch->remessage.empty()) {
            tmpQueue.swap(clch->remessage);
            std::cout << "cur times tmpQueue size:" << tmpQueue.size() << endl;
        }
    }

    std::async(std::launch::async, [this](std::queue<std::string> tmpQueue) {//异步起动让该on_write函数立即返回
            while (!tmpQueue.empty()) {
                shared_const_buffer buffer(tmpQueue.front());
                //async_write(sock_,buffer, bind(&clint::clint_handle_write, this, buffer, _1, _2));
                async_write(sock_, buffer, bind(&clint::clint_handle_write, this, std::string(tmpQueue.front().begin(), tmpQueue.front().end()), _1, _2));
                tmpQueue.pop();
            }   
        }
        ,std::move(tmpQueue));

    
}
void clint::clint_handle_write(std::string buffer, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        //出错将消息重新置为初始态压入队列
        //string tmp(buffer.begin(), buffer.end());
        {
            lock_guard<mutex>relock(clch->remu);
            clch->remessage.push(buffer);
        }
        errorhandle(er);
        return;
    }
    static int count = 0;
    std::cout << "write one message" << count++ << endl;
    //将消息状态置为可丢弃压入队列
    //this->on_write();
}


int clint::headanylize(const Head* head) {
    if (head->type == 'm') {
        return 1;
    }
    else if (head->type == 'f') {
        return 2;
    }
    else if (head->type == 'l') {//登录
        return 3;
    }
    else if (head->type == 'i') {//注册
        return 4;
    }
    else if (head->type == 'a') {//群聊消息
        return 5;
    }
    else if (head->type == 'd') {//加入群聊
        return 6;
    }
    else if (head->type == 'r') {//创建群聊
        return 7;
    }
    else if (head->type == 'z') {//回射服务
        return 8;
    }
    else if (head->type == 't') {
        return 9;
    }
    else {
        return 0;//无法解析包头就回射消息
    }
}
void clint:: changestatus(string& p, unsigned int st) {//这边处理完有可能字符串搞错了等会修bug考虑这里
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
/*boost::asio::error::bad_descriptor      (10009)     在一个已经关闭了的套接字上执行async_receive()
boost::asio::error::operation_aborted    (995)    正在async_receive()异步任务等待时，本端关闭套接字
boost::asio::error::connection_reset    (10054)    正在async_receive()异步任务等待时，远端的TCP协议层发送RESET终止链接，暴力关闭套接字。常常发生于远端进程强制关闭时，操作系统释放套接字资源。
boost::asio::error::eof    (2)    正在async_receive()异步任务等待时，远端关闭套接字，这里跟10054发生的情况似乎一样，但是实际上应该是有区别的，具体神马区别，由回复中jack的说法，这个是远端正常关闭套接字。
ERROR_SEM_TIMEOUT    (121)    信号灯超时时间已到。使用完成端口模型作服务器，当GetQueuedCompletionStatus的时候,会出现此错误。

ERROR_CONNECTION_ABORTED   (1236)   由本地系统终止网络连接。使用完成端口模型作服务器，当GetQueuedCompletionStatus的时候,会出现此错误。*/
int clint::errorhandle(boost::system::error_code& er) {
    if (isdiascard) {//有其他异步任务已经检测出该对象被丢弃了
        return 1;
    }
    std::cout << er.what() << endl;
    if (er != boost::asio::error::operation_aborted)//本端未关闭套接字
    {
        sock_.close();
        isdiascard = true;
        islogout = true;
        std::cout << this->id << "logout" << endl;
        return 0; //retruen 0调用close关闭本端套接字释放资源
    }
    return 1;  //一般错误
}

ip::tcp::socket& clint::sock() {
    return sock_;
}
void clint::ls_file(string &s) {

#ifdef UNIX
    struct dirent* ent = NULL;
    DIR* pDir;

    if ((pDir = opendir(filePath)) == NULL)
    {
        printf("open dir %s failed\n",filePath);
        return;
    }
    try {
        while ((ent = readdir(pDir)) != NULL)
        {
            //printf("the ent->d_reclen is%d the ent->d_type is%d the ent->d_name is%s\n", ent->d_reclen, ent->d_type, ent->d_name);
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            string tmp(ent->d_name);
            if (tmp.find(s) != string::npos) {
                cout << "find:" << tmp << endl;
                s = tmp;
                break;
            }
        }
    }
    catch (std::exception e) {
        cout << __LINE__ << e.what() << endl;
        //connection_manager_.stop(shared_from_this());
    }
    //do_FileRecv_write();
    closedir(pDir);
#endif // UNIX 
#ifdef WIN
    bool is = true;
    try {
        intptr_t handle;
        struct _finddata_t fileinfo;
        handle = _findfirst(filePath, &fileinfo);
        cout <<"filePath:" << filePath << endl;
        do
        {
            //找到的文件的文件名
            if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
            string tmp(fileinfo.name);     
            if(tmp.find(s)!=string::npos){
                cout <<"find:" << tmp << endl;
                s = tmp;
                break;
            }

        } while (!_findnext(handle, &fileinfo));
        _findclose(handle);
    }
    catch (...) {
    }

#endif //WIN
}