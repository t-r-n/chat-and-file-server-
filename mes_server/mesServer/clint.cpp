#include"clint.h"

void clint::on_write_reback(int back,Head he,bool is) {
    if (isdiascard) {
        return;
    }
    if (!is) {
        he.type = 'r';//��������Ϣ����
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
                cout << this->id << ":���ӹر�" << endl;

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
    buf.clear();  //��������ߵ��µĶ�������
    buf.resize(1024);//����������async_read��һֱ��ȡ
    auto self(shared_from_this());
    cout << "reading ing" <<he.length << endl;
    //he.lengthͻȻ����ʲô�����ҵ��������ĳ���
    async_read(sock_, buffer(buf), transfer_exactly(he.length), strand_.wrap([this, self,he](boost::system::error_code er, size_t sz) {
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "���ӹر�" << endl;//�������
                sock_.close();
                islogout = true;
            }
            return;
        }
        int ret = headanylize((const Head*)&he);
        h = (Head*)&he;
        cout << "reading" << endl;
        switch (ret) {
            case 1: {//��Ϣת��
                cout << __LINE__<<endl;
                //��Ϣ/�ļ����͸�˭�Ƿŵ���ͷ�ﻹ������Ϣ��//���ڰ�������񻹵����´����²��ܷŻ���Ϣ�����������߲�����ֱ���öԷ�on_write�Ǳߴ���ֱ��ѹ��������
//��ǰ�汾�ǲ�����  //�����ǲ�����server�Ǳ߰Ѱ�ͷ��΢��һ��ֱ�ӷ����Է��Ͼ��Է����ܵ�����ҲӦ���и��ְ�ͷ
                buf.insert(0, string((char*)&he, sizehead));
                {
                    if (clch) {
#ifdef LOCK_DEBUG
                        cout << __LINE__ << "catching lock:" << "selock" << endl;
#endif
                        lock_guard<mutex> selock(clch->semu);

                        clch->semessage.push(buf);
                        is_have_task = true;//***********************************************************************************************************************
                        semu_cond.notify_all();//���Ѵ���������߳�
                    }
                }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
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
                        semu_cond.notify_all();//���Ѵ���������߳�
                    }
                }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
                break;
            }
            case 3: {//��½��֤
                cout << __LINE__ << endl;
                unsigned int tmppassword = 0;
                {
                    lock_guard<mutex>acc_lock(acc_mutex);
                    if (account.find(h->account) != account.end()) {//�и��˻�
                        tmppassword = account[h->account]->password;
                    }
                    else {
                        on_write_reback(reback::loginfal);//��½ʧ��
                        //self->on_read();
                        self->on_read_content(Head());
                        return;
                    }
                }
                if (tmppassword != h->mima) {//���벻��ȷ;
                    this->on_write_reback(reback::passerror);
                }
                else {//��½�ɹ�//��ȫ�ֵ�clint����һ��     ///*************�����ִ��������ߵ�accountռ�Ź�ϣ�����Դ���ǲ����ʵ�
                    //�Ӳ�����?
                    //������û���Ѿ�����
                    bool isloogin = false;
                    {
                        lock_guard<mutex>cur_log_lock(cur_account_ptr_mutex);
#ifdef DEBUG
                        cout << __LINE__ << "�õ���cur_log_lock" << endl;
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
                        //�Ͽ�������
                    }
                    else {
                        clch = account[h->account];  //֮����һ�����account���sharer����ptr�������cur_ptr��û�г�ͻ
                        on_write_reback(reback::logsucc);
                        cout << "��½�ɹ�" << endl;
                        //is_have_task = true;//***********************************************************************************************************************
                        //semu_cond.notify_all();//���Ѵ���������߳�
                        //is_have_task1 = true;//����Ϊ�˿�����û�в�����ʱ����Ϣ���ڿ��Բ�����
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
                        cout << "ע��ʧ��" << endl;
                    }
                }
                if (issuc) {
                    clch->account = h->account;
                    clch->password = h->mima;
                    on_write_reback(reback::loginsucc);
                    cout << "ע��ɹ�" << endl;
                }
                break;
            }
            case 5: {
                cout << __LINE__ << endl; //������Ⱥ����Ⱥ����ӵ�еĳ�Աѹ��semessage
                break;
            }
            case 6: {//����Ⱥ��
                {
                    
                    lock_guard<mutex>talkRoomLock(talkRoomQueue_mutex);
                    if (talkRoomQueue.find(h->id) == talkRoomQueue.end()) {//û�ҵ���Ⱥ��
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
                    on_write_reback(reback::BuildTalkRoomSuc, he, true);//����Ⱥ�Ĵ����ɹ�����Ϣ
                }
                break;
            }
            case 7: {//����Ⱥ��
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
                    on_write_reback(reback::BuildTalkRoomSuc, he, true);//����Ⱥ�Ĵ����ɹ�����Ϣ
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
    buf.clear();  //��������ߵ��µĶ�������
    buf.resize(1024);//����������async_read��һֱ��ȡ
    auto self(shared_from_this());
    async_read(sock_, buffer(buf), transfer_exactly(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        if (er) {//��������
            int ret = errorhandle(er);
            if (ret == 0) {//��������ʱ,����δ�رգ��Ǿ�close�ͷ��ڴ�
                cout << "���ӹر�" << endl;//�������
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
            std::cout << "��������Ϣֻ�а�ͷ������Ϊ����,����Ӧ�����socket������" << std::endl;
            self->on_read_plus();
            
        }
    }));
}
#if 0
void clint::on_read() {
    if (isdiascard) {
        return;
    }
    buf.clear();  //��������ߵ��µĶ�������
    buf.resize(1024);//����������async_read��һֱ��ȡ
    auto self(shared_from_this());
    //����������������յ����ܽ�����ͷ������
    async_read(sock_, buffer(buf), transfer_at_least(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        //async_read(sock_, buffer(buf), transfer_at_least(1), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {

        //async_read(sock_, buffer(buf),"\n", [=](boost::system::error_code er, size_t sz) {
            //transfer_all()��ֱ����ȡ��1024�ֽ�Ҳ����˵��buffer�����ŷ���
            //transfer_at_least(n)������������n�ֽ�ʱ��ȡ����������ܵ����ݵĻ����͵�ʱ�򻺳�������Ŀ��ַ�Ҳ����ȥ����Ҫ����һ�£�Ҳ����˵Ҫ�Լ�����ÿ��
            //ÿ�η�����Ľ�β��ǻ�涨һ�����ʵ�Ҫ���͵Ļ�������Сbuffer(buf,���ʵĴ�С)
        if (er) {
            //��Ҫ���ͻ��˻����Ƿ��ͳɹ�û�гɹ������Ƿ������Է����ǿͻ��˵Ŀͻ��˵���������Ӧ�Ľ���ش�����ʲô������ʱ�����û�����
            //�ر����Ӿ���clintchar�ڹ�ϣ���ﱣ���ţ���ǰ������Ϊ��������
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "���������ر�" << endl;//�������
                sock_.close();
                islogout = true;
            }
            else {
                on_write_reback(reback::readerror);
                self->on_read();
            }
            return;
        }
        //cout << "���յ��Ķ�����" << buf << endl;
        string headme(buf.begin(), buf.begin() + sizehead);
        memcpy(ttmphead, headme.c_str(), sizehead);
        h = (Head*)ttmphead;
        int ret = headanylize(h);//0����Ϣͷ��ʽ����1��Ϣ��2�ļ�����
        if (ret == 1) {//��Ϣ

#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "���յ���Ϣ�ĳ���" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "���յ�����Ϣ" << tmp << endl;
#endif
            //��Ϣ/�ļ����͸�˭�Ƿŵ���ͷ�ﻹ������Ϣ��//���ڰ�������񻹵����´����²��ܷŻ���Ϣ�����������߲�����ֱ���öԷ�on_write�Ǳߴ���ֱ��ѹ��������
            //��ǰ�汾�ǲ�����  //�����ǲ�����server�Ǳ߰Ѱ�ͷ��΢��һ��ֱ�ӷ����Է��Ͼ��Է����ܵ�����ҲӦ���и��ְ�ͷ
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else if (ret == 2) {//�ļ�����
#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "���յ���Ϣ�ĳ���" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "���յ�����Ϣ" << tmp << endl;
#endif
            //��Ϣ/�ļ����͸�˭�Ƿŵ���ͷ�ﻹ������Ϣ��//���ڰ�������񻹵����´����²��ܷŻ���Ϣ�����������߲�����ֱ���öԷ�on_write�Ǳߴ���ֱ��ѹ��������
            //��ǰ�汾�ǲ�����  //�����ǲ�����server�Ǳ߰Ѱ�ͷ��΢��һ��ֱ�ӷ����Է��Ͼ��Է����ܵ�����ҲӦ���и��ְ�ͷ
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);

                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else if (ret == 3) {//��¼����
            unsigned int tmppassword = 0;
            {
                lock_guard<mutex>acc_lock(acc_mutex);
                if (account.find(h->account) != account.end()) {//�и��˻�
                    tmppassword = account[h->account]->password;
                }
                else {
                    on_write_reback(reback::loginfal);//��½ʧ��
                    self->on_read();
                    return;
                }
            }
            if (tmppassword != h->mima) {//���벻��ȷ;
                this->on_write_reback(reback::passerror);
            }
            else {//��½�ɹ�//��ȫ�ֵ�clint����һ��     ///*************�����ִ��������ߵ�accountռ�Ź�ϣ�����Դ���ǲ����ʵ�
                //�Ӳ�����?
                //������û���Ѿ�����
                bool isloogin = false;
                {
                    lock_guard<mutex>cur_log_lock(cur_account_ptr_mutex);
#ifdef DEBUG
                    cout << __LINE__ << "�õ���cur_log_lock" << endl;
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
                    //�Ͽ�������
                }
                else {
                    clch = account[h->account];  //֮����һ�����account���sharer����ptr�������cur_ptr��û�г�ͻ
                    on_write_reback(reback::logsucc);
                    cout << "��½�ɹ�" << endl;
                    //is_have_task = true;//***********************************************************************************************************************
                    //semu_cond.notify_all();//���Ѵ���������߳�
                    //is_have_task1 = true;//����Ϊ�˿�����û�в�����ʱ����Ϣ���ڿ��Բ�����
                    //semu_cond1.notify_all();
                }
            }
        }
        else if (ret == 4) {//ע��newһ��clintchar
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
                    cout << "ע��ʧ��" << endl;
                }
            }
            if (issuc) {
                clch->account = h->account;
                clch->password = h->mima;
                on_write_reback(reback::loginsucc);
                cout << "ע��ɹ�" << endl;
            }
        }
        else if (ret == 5) {//Ⱥ����Ϣ
            {//Ӧ������һ�����߳�����������ѭ��������̫ռcpu��
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else if (ret == 6) {//�ѵ�ǰӵ�е��ļ��ļ������ͻ�ȥ
   //���ڲ��ҵľ��
#ifdef WIN
            string eassy;
            bool is = true;
            try {
                long handle;
                struct _finddata_t fileinfo;
                //��һ�β���
                handle = _findfirst(inPath.c_str(), &fileinfo);
                do
                {
                    //�ҵ����ļ����ļ���
                    if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
                    eassy += string(fileinfo.name) + "\n";
                    //printf("%s\n", fileinfo.name);

                } while (!_findnext(handle, &fileinfo));
                _findclose(handle);
            }
            catch (...) {
                is = false;
                on_write_reback(reback::readerror);
                cout << "��ȡ�����ļ�error" << endl;
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
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }
#endif //WIN
#ifdef LINUX
            //��ָ��Ŀ¼opendir�õ�Ŀ¼���
            DIR* dir = opendir("./FileRecv");


            //struct dirent�ṹ������������洢��Ŀ¼��
            struct dirent* entry;


            //Ȼ��ͨ��whileѭ������readdir����ȡĿ¼�е�����
            while ((entry = readdir(dir)) != 0) {

                //��ȡ�ýṹ������ĳ�Ա����d_name�͵õ��˴�ɨ����ļ���Ȼ����ʹ��sprintf���������ļ�����·��
                printf("%s\n", entry->d_name);
                char* newpath;
                sprintf(newpath, "%s%s", filepath, entry->d_name);
                printf("%s\n", newpath);

            }

            //���ر�Ŀ¼���closedir
            closedir(dir);

#endif //LINUX
        }
        else if (ret==7) {//����Ⱥ��

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
                on_write_reback(reback::BuildTalkRoomSuc,he,true);//����Ⱥ�Ĵ����ɹ�����Ϣ
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
                    semu_cond.notify_all();//���Ѵ���������߳�
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else {
                async_write(sock_, buffer(buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                    if (er) {
                        int ret = errorhandle(er);
                        if (ret == 0) {
                            cout << this->id << ":���ӹر�" << endl;
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
                cout << "�޷�������ͷ�Ǳ�׼�ͻ��˷��͵���Ϣ" << endl;
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
        cout << __LINE__ << "�õ���" << endl;
#endif // DEBUG

        if (clch->remessage.size() > 0) {
            reme = clch->remessage.front();
            //����Ϣ״̬��Ϊ�Ѵ���
            clch->remessage.pop();
        }
        else {
            return;//���һ����֤��ֹ���ֲ�ȷ����
        }
    }
#ifdef DEBUG
    cout << "id:" << this->id << ":" << this->clch->account << "��ʼ�첽��д" << reme << endl;
#endif
    async_write(sock_, buffer(reme, reme.size()), transfer_at_least(reme.size() - 1), bind(&clint::clint_handle_write, this, reme, _1, _2));//���ռλ�����ܻ���Щ����
}
void clint::clint_handle_write(string p, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        int ret = errorhandle(er);
        if (ret == 0) {
            cout << "���ӹر�" << endl;
            sock_.close();
            islogout = true;
        }
        else {
            on_write_reback(reback::writeerror);
        }
        //������Ϣ������Ϊ��ʼ̬ѹ�����
        changestatus(p, 0);
        {
            lock_guard<mutex>relock(clch->remu);
#ifdef DEBUG
            cout << __LINE__ << "�õ���" << endl;
#endif // DEBUG
            clch->remessage.push(p);
        }
        cout << "on_write error";
        return;
    }
    //����Ϣ״̬��Ϊ�ɶ���ѹ�����
    this->on_write();
}
int clint::headanylize(const Head* head) {
    if (head->type == 'm') {
        return 1;
    }
    else if (head->type == 'f') {
        return 2;
    }
    else if (head->type == 'l') {//��¼
        return 3;
    }
    else if (head->type == 'i') {//ע��
        return 4;
    }
    else if (head->type == 'a') {//Ⱥ����Ϣ
        return 5;
    }
    else if (head->type == 'd') {//����Ⱥ��
        return 6;
    }
    else if (head->type == 'r') {//����Ⱥ��
        return 7;
    }
    else if (head->type == 'z') {//�������
        return 8;
    }
    else if (head->type == 't') {
        return 9;
    }
    else {
        return 0;//�޷�������ͷ�ͻ�����Ϣ
    }
}
void clint:: changestatus(string& p, unsigned int st) {//��ߴ������п����ַ�������˵Ȼ���bug��������
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
/*boost::asio::error::bad_descriptor      (10009)     ��һ���Ѿ��ر��˵��׽�����ִ��async_receive()
boost::asio::error::operation_aborted    (995)    ����async_receive()�첽����ȴ�ʱ�����˹ر��׽���
boost::asio::error::connection_reset    (10054)    ����async_receive()�첽����ȴ�ʱ��Զ�˵�TCPЭ��㷢��RESET��ֹ���ӣ������ر��׽��֡�����������Զ�˽���ǿ�ƹر�ʱ������ϵͳ�ͷ��׽�����Դ��
boost::asio::error::eof    (2)    ����async_receive()�첽����ȴ�ʱ��Զ�˹ر��׽��֣������10054����������ƺ�һ��������ʵ����Ӧ����������ģ��������������ɻظ���jack��˵���������Զ�������ر��׽��֡�
ERROR_SEM_TIMEOUT    (121)    �źŵƳ�ʱʱ���ѵ���ʹ����ɶ˿�ģ��������������GetQueuedCompletionStatus��ʱ��,����ִ˴���

ERROR_CONNECTION_ABORTED   (1236)   �ɱ���ϵͳ��ֹ�������ӡ�ʹ����ɶ˿�ģ��������������GetQueuedCompletionStatus��ʱ��,����ִ˴���*/
int clint::errorhandle(boost::system::error_code& er) {
    if (isdiascard) {//�������첽�����Ѿ������ö��߱�������
        return 1;
    }
    //int ervalue = er.value();
    //if (ervalue == 10009 || ervalue == 2 || ervalue == 995 || ervalue == 10054 || ervalue == 2) {//
    //    isdiascard = true;
    //    return 0;
    //}
    if (er != boost::asio::error::operation_aborted)//����δ�ر��׽���
    {
        isdiascard = true;
        return 0; //retruen 0����close�رձ����׽����ͷ���Դ
    }
    return 1;  //һ�����
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
        //��һ�β���
        handle = _findfirst("E:/trn_project/trhttpserver/FileRecv/*", &fileinfo);
        cout << "find:" << s <<"s.size():"<<s.size() << endl;
        do
        {
            //�ҵ����ļ����ļ���
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