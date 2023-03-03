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
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << this->id << ":链接关闭" << endl;

                sock_.close();
                islogout = true;
            }
            else {
                on_write_reback(reback::writeerror);
            }
            /*        cout << "async_write error" << endl;
                    sock_.close();
                    islogout = true;*/

            return;
        }
        cout << "send ed:" << he.type << " " << he.status << " " << he.length << endl;
    });
}
void clint::on_read_content(Head he) {
    if (isdiascard) {
        return;
    }
    buf.clear();  //好像是这边导致的断言问题
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    cout << "reading ing" <<he.length << endl;
    //he.length突然错误什么问题找到问题中文出错
    async_read(sock_, buffer(buf), transfer_exactly(he.length), strand_.wrap([this, self,he](boost::system::error_code er, size_t sz) {
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "链接关闭" << endl;//网络错误
                sock_.close();
                islogout = true;
            }
            return;
        }
        int ret = headanylize((const Head*)&he);
        h = (Head*)&he;
        cout << "reading" << endl;
        switch (ret) {
            case 1: {//消息转发
                cout << __LINE__<<endl;
                //消息/文件发送给谁是放到包头里还是在消息里//放在包里这好像还得重新处理下才能放回消息队列里或者这边不处理直接让对方on_write那边处理直接压倒对列里
//当前版本是不处理  //必须是不处理server那边把包头稍微改一下直接发给对方毕竟对方接受的数据也应该有各种包头
                buf.insert(0, string((char*)&he, sizehead));
                {
                    if (clch) {
#ifdef LOCK_DEBUG
                        cout << __LINE__ << "catching lock:" << "selock" << endl;
#endif
                        lock_guard<mutex> selock(clch->semu);

                        clch->semessage.push(buf);
                        is_have_task = true;//***********************************************************************************************************************
                        semu_cond.notify_all();//唤醒处理任务的线程
                    }
                }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
                break;
            }
            case 2: {
                cout <<"h->type;" << h->type << "he->account" << h->account << "h->sen:" << h->sendto << "name:" << buf << endl;
                string tmpbuf(buf.data(),h->length);
                
                buf = tmpbuf;
                ls_file(buf);
                ((Head*)&he)->length = buf.size();
                buf.insert(0, string((char*)&he, sizehead));
                {
                    if (clch) {
                        lock_guard<mutex> selock(clch->semu);
                        clch->semessage.push(buf);
                        is_have_task = true;//***********************************************************************************************************************
                        semu_cond.notify_all();//唤醒处理任务的线程
                    }
                }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
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
                else {//登陆成功//把全局的clint拷贝一份     ///*************当出现大量已下线的account占着哈希表的资源这是不合适的
                    //加不加锁?
                    //看看有没有已经在线
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
                        //断开链接吗？
                    }
                    else {
                        clch = account[h->account];  //之后检查一下这个account里的sharer——ptr和上面的cur_ptr有没有冲突
                        on_write_reback(reback::logsucc);
                        cout << "登陆成功" << endl;
                        //is_have_task = true;//***********************************************************************************************************************
                        //semu_cond.notify_all();//唤醒处理任务的线程
                        //is_have_task1 = true;//这里为了看看有没有不在线时的消息现在可以不用了
                        //semu_cond1.notify_all();
                    }
                }
                break;
            }
            case 4: {
                cout << __LINE__ << endl;
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
                        cout << "注册失败" << endl;
                    }
                }
                if (issuc) {
                    clch->account = h->account;
                    clch->password = h->mima;
                    on_write_reback(reback::loginsucc);
                    cout << "注册成功" << endl;
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
                    talkRoomQueue[h->id] = { h->account };
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
    buf.clear();  //好像是这边导致的断言问题
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    async_read(sock_, buffer(buf), transfer_exactly(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        if (er) {//发生错误
            int ret = errorhandle(er);
            if (ret == 0) {//发生错误时,本端未关闭，那就close释放内存
                cout << "链接关闭" << endl;//网络错误
                sock_.close();
                islogout = true;
            }
            return;
        }
        Head h = getHead(buf);
        int ret = headanylize((const Head*)buf.c_str());
        if (h.length >= 0&&h.length<1024) {
            self->on_read_content(h);
            return;
        }
        else {
            std::cout << "读到的消息只有包头，功能为扩充,这里应该清空socket缓冲区" << std::endl;
            self->on_read_plus();
            
        }
    }));
}
#if 0
void clint::on_read() {
    if (isdiascard) {
        return;
    }
    buf.clear();  //好像是这边导致的断言问题
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    //如果服务器不允许收到不能解析包头的数据
    async_read(sock_, buffer(buf), transfer_at_least(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        //async_read(sock_, buffer(buf), transfer_at_least(1), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {

        //async_read(sock_, buffer(buf),"\n", [=](boost::system::error_code er, size_t sz) {
            //transfer_all()会直到读取到1024字节也就是说把buffer读满才返回
            //transfer_at_least(n)当缓冲区大于n字节时读取但不处理接受的数据的话发送的时候缓冲区后面的空字符也发过去了需要处理一下，也就是说要自己定义每次
            //每次发送完的结尾标记或规定一个合适的要发送的缓冲区大小buffer(buf,合适的大小)
        if (er) {
            //还要给客户端回馈是否发送成功没有成功报告是服务器对方还是客户端的客户端的问题有相应的解决重传还是什么、、及时保存用户数据
            //关闭链接就行clintchar在哈希表里保存着，当前对象标记为废弃就行
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "链接主动关闭" << endl;//网络错误
                sock_.close();
                islogout = true;
            }
            else {
                on_write_reback(reback::readerror);
                self->on_read();
            }
            return;
        }
        //cout << "接收到的东西：" << buf << endl;
        string headme(buf.begin(), buf.begin() + sizehead);
        memcpy(ttmphead, headme.c_str(), sizehead);
        h = (Head*)ttmphead;
        int ret = headanylize(h);//0，消息头格式错误，1消息，2文件传输
        if (ret == 1) {//消息

#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "接收到消息的长度" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "接收到的消息" << tmp << endl;
#endif
            //消息/文件发送给谁是放到包头里还是在消息里//放在包里这好像还得重新处理下才能放回消息队列里或者这边不处理直接让对方on_write那边处理直接压倒对列里
            //当前版本是不处理  //必须是不处理server那边把包头稍微改一下直接发给对方毕竟对方接受的数据也应该有各种包头
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else if (ret == 2) {//文件传输
#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "接收到消息的长度" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "接收到的消息" << tmp << endl;
#endif
            //消息/文件发送给谁是放到包头里还是在消息里//放在包里这好像还得重新处理下才能放回消息队列里或者这边不处理直接让对方on_write那边处理直接压倒对列里
            //当前版本是不处理  //必须是不处理server那边把包头稍微改一下直接发给对方毕竟对方接受的数据也应该有各种包头
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);

                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else if (ret == 3) {//登录环节
            unsigned int tmppassword = 0;
            {
                lock_guard<mutex>acc_lock(acc_mutex);
                if (account.find(h->account) != account.end()) {//有该账户
                    tmppassword = account[h->account]->password;
                }
                else {
                    on_write_reback(reback::loginfal);//登陆失败
                    self->on_read();
                    return;
                }
            }
            if (tmppassword != h->mima) {//密码不正确;
                this->on_write_reback(reback::passerror);
            }
            else {//登陆成功//把全局的clint拷贝一份     ///*************当出现大量已下线的account占着哈希表的资源这是不合适的
                //加不加锁?
                //看看有没有已经在线
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
                    //断开链接吗？
                }
                else {
                    clch = account[h->account];  //之后检查一下这个account里的sharer——ptr和上面的cur_ptr有没有冲突
                    on_write_reback(reback::logsucc);
                    cout << "登陆成功" << endl;
                    //is_have_task = true;//***********************************************************************************************************************
                    //semu_cond.notify_all();//唤醒处理任务的线程
                    //is_have_task1 = true;//这里为了看看有没有不在线时的消息现在可以不用了
                    //semu_cond1.notify_all();
                }
            }
        }
        else if (ret == 4) {//注册new一个clintchar
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
                    cout << "注册失败" << endl;
                }
            }
            if (issuc) {
                clch->account = h->account;
                clch->password = h->mima;
                on_write_reback(reback::loginsucc);
                cout << "注册成功" << endl;
            }
        }
        else if (ret == 5) {//群聊消息
            {//应该启动一个新线程这样在这里循环加锁可太占cpu了
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else if (ret == 6) {//把当前拥有的文件文件名发送回去
   //用于查找的句柄
#ifdef WIN
            string eassy;
            bool is = true;
            try {
                long handle;
                struct _finddata_t fileinfo;
                //第一次查找
                handle = _findfirst(inPath.c_str(), &fileinfo);
                do
                {
                    //找到的文件的文件名
                    if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
                    eassy += string(fileinfo.name) + "\n";
                    //printf("%s\n", fileinfo.name);

                } while (!_findnext(handle, &fileinfo));
                _findclose(handle);
            }
            catch (...) {
                is = false;
                on_write_reback(reback::readerror);
                cout << "获取本地文件error" << endl;
            }
            if (is) {
                h->type = 'd';
                h->length = eassy.size();
                h->sendto = h->account;
                h->account = 0;
            }
            string bufd = string((char*)h, sizeof(Head)) + eassy;
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(bufd);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }
#endif //WIN
#ifdef LINUX
            //打开指定目录opendir得到目录句柄
            DIR* dir = opendir("./FileRecv");


            //struct dirent结构体变量，用来存储子目录项
            struct dirent* entry;


            //然后通过while循环不断readdir，获取目录中的内容
            while ((entry = readdir(dir)) != 0) {

                //获取该结构体变量的成员函数d_name就得到了待扫描的文件，然后在使用sprintf函数加入文件绝对路径
                printf("%s\n", entry->d_name);
                char* newpath;
                sprintf(newpath, "%s%s", filepath, entry->d_name);
                printf("%s\n", newpath);

            }

            //最后关闭目录句柄closedir
            closedir(dir);

#endif //LINUX
        }
        else if (ret==7) {//创建群聊

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
                on_write_reback(reback::BuildTalkRoomSuc,he,true);//返回群聊创建成功的消息
            }

        }
        else if (ret == 8) {
        
            {
                shared_ptr<clintchar>pr;
                {
                    lock_guard<mutex>ll(acc_mutex);
                    pr = account[h->account];
                }
                    lock_guard<mutex> selock(pr->semu);
                    pr->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else {
                async_write(sock_, buffer(buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                    if (er) {
                        int ret = errorhandle(er);
                        if (ret == 0) {
                            cout << this->id << ":链接关闭" << endl;
                            sock_.close();
                            islogout = true;
                        }
                        else {
                            on_write_reback(reback::writeerror);
                        }
                        return;
                    }
                    //cout << "send ed:" << he.type << " " << he.status << " " << he.length << endl;
                });
            
            //on_write1(reback::readerror);
#ifdef DEBUG
                cout << "无法解析包头非标准客户端发送的消息" << endl;
#endif // DEBUG
        }
        self->on_read();
        }));
}
#endif
void clint::on_write() {
    if (isdiascard) {
        return;
    }
    reme.clear();
    reme.resize(1024);
    {
        lock_guard<mutex> relock(clch->remu);
#ifdef DEBUG
        cout << __LINE__ << "拿到锁" << endl;
#endif // DEBUG

        if (clch->remessage.size() > 0) {
            reme = clch->remessage.front();
            //将消息状态置为已处理
            clch->remessage.pop();
        }
        else {
            return;//多加一层认证防止各种不确定性
        }
    }
#ifdef DEBUG
    cout << "id:" << this->id << ":" << this->clch->account << "开始异步读写" << reme << endl;
#endif
    async_write(sock_, buffer(reme, reme.size()), transfer_at_least(reme.size() - 1), bind(&clint::clint_handle_write, this, reme, _1, _2));//这边占位符可能还有些问题
}
void clint::clint_handle_write(string p, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        int ret = errorhandle(er);
        if (ret == 0) {
            cout << "链接关闭" << endl;
            sock_.close();
            islogout = true;
        }
        else {
            on_write_reback(reback::writeerror);
        }
        //出错将消息重新置为初始态压入队列
        changestatus(p, 0);
        {
            lock_guard<mutex>relock(clch->remu);
#ifdef DEBUG
            cout << __LINE__ << "拿到锁" << endl;
#endif // DEBUG
            clch->remessage.push(p);
        }
        cout << "on_write error";
        return;
    }
    //将消息状态置为可丢弃压入队列
    this->on_write();
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
    if (isdiascard) {//有其他异步任务已经检测出该对线被丢弃了
        return 1;
    }
    //int ervalue = er.value();
    //if (ervalue == 10009 || ervalue == 2 || ervalue == 995 || ervalue == 10054 || ervalue == 2) {//
    //    isdiascard = true;
    //    return 0;
    //}
    if (er != boost::asio::error::operation_aborted)//本端未关闭套接字
    {
        isdiascard = true;
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

    if ((pDir = opendir("./FileRecv")) == NULL)
    {
        //printf("open dir %s failed\n", pszBaseDir);
        contentbuf += "open dir %s failed\n";
        do_FileRecv_write();
        return;
    }
    try {
        while ((ent = readdir(pDir)) != NULL)
        {
            //printf("the ent->d_reclen is%d the ent->d_type is%d the ent->d_name is%s\n", ent->d_reclen, ent->d_type, ent->d_name);
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            contentbuf += ent->d_name;
            contentbuf.append(" ");
        }
    }
    catch (std::exception e) {
        cout << __LINE__ << e.what() << endl;
        connection_manager_.stop(shared_from_this());
    }
    do_FileRecv_write();
    closedir(pDir);
#endif // UNIX 
#ifdef WIN
    //string eassy;
    bool is = true;
    try {
        long handle;
        struct _finddata_t fileinfo;
        //第一次查找
        handle = _findfirst("E:/trn_project/trhttpserver/FileRecv/*", &fileinfo);
        cout << "find:" << s <<"s.size():"<<s.size() << endl;
        do
        {
            //找到的文件的文件名
            if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
            string tmp(fileinfo.name);
 /*           for (int i = 0; fileinfo.name[i] != '/0' && i < 256; ++i) {
                tmp += fileinfo.name[i];
            }*/
            cout << tmp << endl;
            
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