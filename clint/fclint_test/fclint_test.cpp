// fclint_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include"fclint.h"
#include<thread>
#include<memory>
#include"thread_pool.h"
using namespace std;
int main(int argc,char*argv[])
{
    int count = 0;
    if (argc < 2) {
        cout << "par error" << endl;
        return 0;
    }
    ilovers::TaskExecutor executor{ 5 };

    //std::future<void> ff = executor.commit(f);
    //std::future<int> fg = executor.commit(G{});
    //std::future<std::string> fh = executor.commit([]()->std::string { std::cout << "hello, h !" << std::endl; return "hello,fh !"; });

   
    //ff.get();
    //std::cout << fg.get() << " " << fh.get() << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    //executor.restart();    // 重启任务
    //executor.commit(f).get();    //


    vector<shared_ptr<fclint>>fc_ptr_group;
    //fclint fc((char*)"E:/trn_project/tmpfile/ad.mp4");
    if (string(argv[1]).find("s")!=string::npos) {
        if (argc == 2) {//只发送一个文件
            fclint fc(argv[2]);
            //fc.sendto = stoi(argv[3]);
            //fc.acc = stoi(argv[4]);
            fc.sendto = 10086;
            fc.acc = 10086;
            //fclint fc((char*)"E:/trn_project/tmpfile/cpr.pdf");
            //cout << "sendto:" << fc.sendto << " acc:" << fc.acc << endl;
            fc.do_read();
        }
        else {
            for (int i = 2; i < argc; ++i) {
                fc_ptr_group.push_back(make_shared<fclint>(argv[i]));
            }
            for (int i = 0; i < fc_ptr_group.size();++i) {
                executor.commit([fc_ptr_group,i](){
                    //cout << "第"<<i<<"个线程任务启动" << endl;
                    fc_ptr_group[i]->do_read();
                    });
            }
        }
    }
    else if (string(argv[1]).find("r") != string::npos) {

        if (argc == 2) {//只发送一个文件
            fclint fc;
            //fc.sendto = stoi(argv[3]);
            //fc.acc = stoi(argv[4]);
            fc.sendto = 10086;
            fc.acc = 10086;
            fc.do_recv(argv[2]);
        }
        else {
            for (int i = 2; i < argc; ++i) {
                fc_ptr_group.push_back(make_shared<fclint>());
                executor.commit([argv, i, fc_ptr_group]() {
                    //cout << "第" << i << "个线程任务启动" << endl;
                    fc_ptr_group.back()->do_recv(argv[i]);
                    });
            }
        }
    }
    else if (string(argv[1]).find("l") != string::npos) {
        fclint fc;
        fc.ls_file();
    }
    else {
        cout << "par error" << endl;
    }
    executor.join();

}

