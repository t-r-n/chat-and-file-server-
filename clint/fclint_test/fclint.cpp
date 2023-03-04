#include"fclint.h"
#define DEBUG
fclint::fclint(char* path) {
#ifdef DEBUG
	//ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("43.143.152.92"), 8010);
	ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("127.0.0.1"), 8011);
	//ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("219.217.199.110"), 8010);
#else

	try {
		ip::tcp::resolver resolver(service);
		ip::tcp::resolver::query query("www.fwtrntr.fun", "8010");
		//ip::tcp::resolver::query query("127.0.0.1", "8009");
		ip::tcp::resolver::iterator iter = resolver.resolve(query);  
		ep = make_shared<ip::tcp::endpoint>(*iter);
		std::cout << ep->address().to_string() << std::endl;
	}
	catch (boost::system::system_error e) {
		std::
			cout << e.what() << endl;
		return;
	}
#endif
	sock_ = make_shared<ip::tcp::socket>(service);
	//ep->set_option(tcp::no_delay(true))
	//sock_->set_option(ip::tcp::no_delay(true));
	sock_->connect(*ep);
	string tmp(path);
	int pos=tmp.rfind('/');
	filename = tmp.substr(pos + 1);
	std::cout << filename << endl;
	
	//setlocale(LC_ALL, "Chinese-simplified");//设置中文环境
	infile.open(path, std::ios::in | std::ios::binary);
	
	//infile.open( , std::ios::in | std::ios::binary);
	char buf[4096];
	while (1) {
		int len=infile.read(buf, 4096).gcount();
		if (len <= 0)break;
		content_buf += string(buf, len);
		if (content_buf.size() > INT_MAX / 2) {
			infile.close();
			std::cout << "file so large" << endl;
			return;
		}
	}
	std::cout << "contlength" << content_buf.size() << endl;
	infile.close();

	
}
fclint::fclint() {
#ifdef DEBUG
	//ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("43.143.152.92"), 8010);
	ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("127.0.0.1"), 8011);
	//ep = make_shared<ip::tcp::endpoint>(ip::address::from_string("219.217.199.110"), 8010);
#else
	try {
		ip::tcp::resolver resolver(service);
		ip::tcp::resolver::query query("www.fwtrntr.fun", "8010");
		ip::tcp::resolver::iterator iter = resolver.resolve(query);
		ep = make_shared<ip::tcp::endpoint>(*iter);
		std::cout << ep->address().to_string() << std::endl;
	}
	catch (boost::system::system_error e) {
		std::cout << e.what() << endl;
		return;
}
#endif
	sock_ = make_shared<ip::tcp::socket>(service);
	//sock_->set_option(ip::tcp::no_delay(true));
	sock_->connect(*ep);
	
}
void fclint::do_write() {
	//auto self(shared_from_this());
	async_write(*sock_, buffer(content_buf,content_buf.size()),[this](boost::system::error_code er, std::size_t) {
		if (er) {
			std::cout << er.what() << endl;
			sock_->shutdown(boost::asio::socket_base::shutdown_send, er);
			stop();
			return;
		}
		else {
			std::cout << "发送成功" << endl;
			stop();
		}
		});
}
void fclint::do_read() {
	Fhead fh;
	fh.type = 'r';
	//fh.acc = this->acc;
	//fh.sendto = this->sendto;
	fh.acc = 10086;
	fh.sendto = 10086;

	fh.content_length = content_buf.size();
	//fh.filename = (char*)filename.c_str();
#ifdef UNIX_SERVER
	filename=CharConvert::GBKToUTF8(filename.c_str());
#endif
	strncpy_s(fh.filename, filename.c_str(), filename.size());
	//fh.sizeoffilename = filename.size();
	string headbuf((char*)&fh, sizeofFhead);
	sock_->write_some(buffer(headbuf));
	//std::cout << "发送文件头" << endl;
	string tmpbuf;
	tmpbuf.clear();
	tmpbuf.resize(1024);
	sock_->read_some(buffer(tmpbuf));
	//std::cout << tmpbuf << endl;
	boost::asio::io_context::work work(service);//不让run退出
	if (tmpbuf.find("ok") != string::npos) {
		//std::cout << "调用do_write" << endl;
		do_write();
	}
	//std::cout << "到这" << endl;
	service.run();
	std::cout << "任务结束" << endl;
	exit(1);
}
void fclint::do_recv(string filename1) {
	Fhead fh;
	fh.type = 's';
	fh.acc = this->acc;
	fh.sendto = this->sendto;
	//fh.content_length = content_buf.size();
	//fh.filename = (char*)filename.c_str();
	filename = filename1;
#ifdef UNIX_SERVER
	filename1 = CharConvert::GBKToUTF8(filename1.c_str());
#endif
	//int pos=0;
	//if ((pos = filename.rfind(".trtmp")) != string::npos) {
	//	infile.open(filename, std::ios::in);
	//	if (outfile.is_open()) {
	//		string tmp;
	//		getline(infile, tmp);
	//		try {
	//			fh.content_length=stoi(tmp);
	//		}
	//		catch (...) {
	//			return;
	//		}
	//	}
	//}
	strncpy_s(fh.filename, filename1.c_str(), filename1.size());
	//fh.sizeoffilename = filename.size();
	string headbuf((char*)&fh, sizeofFhead);
	sock_->write_some(buffer(headbuf));
	//std::cout << "发送文件头" << endl;

	tmpbuf.clear();
	tmpbuf.resize(1024);
	//cout << "读取" << sock_->read_some(buffer(tmpbuf)) << "个字节" << endl;
	async_read(*sock_, buffer(tmpbuf, sizeofFhead), transfer_exactly(sizeofFhead), [this](boost::system::error_code er, size_t sz) {
		if (er) {
			sock_->shutdown(boost::asio::socket_base::shutdown_receive, er);
			stop();
			return;
		}
		//std::cout << tmpbuf << endl;
		boost::asio::io_context::work work(service);//不让run退出
		//char tmp[sizeofFhead];
		//strncpy_s(tmp, tmpbuf.c_str(), sizeofFhead);
		//content_length = ((Fhead*)tmp)->content_length;
		content_length = ((Fhead*)tmpbuf.c_str())->content_length;
		//std::cout <<"返回数据包的文件大小" << content_length << endl;
		std::cout << "调用do_content_recv" << endl;
		do_content_recv();
		});

	
	//std::cout << "到这" << endl;
	service.run();
}
void fclint::do_content_recv() {
	if (init) {
		//打开文件描述符
		char tmp[1024];
		unsigned seed;  // Random generator seed
	// Use the time function to get a "seed” value for srand
		seed = time(0);
		srand(seed);
		// Now generate and print three random numbers
		//cout << rand() << " ";
		snprintf(tmp, sizeof(tmp), "./F_%d", rand() % 10000);
		filename.insert(0, string(tmp));
		outfile.open(filename, ios::out | ios::binary | ios::trunc);
		init = false;
		if (outfile.is_open()) {
			//std::cout << "初始化结束" << endl;
		}
		
		starttime_seconds = std::chrono::system_clock::now();
		//contentbuf.resize(65536);//放着还不行必须每次重新指定大小
	}
	//contentbuf.clear();
	content_buf.resize(65536);
	//starttime_read = std::chrono::system_clock::now();
	async_read(*sock_, buffer(content_buf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
		if (er) {
			sock_->shutdown(boost::asio::socket_base::shutdown_receive, er);
			stop();
			return;
		}else {
			iocount++;


			//cout << "进行一次异步读取任务" << endl;
			if (outfile.is_open()) {
				outfile.write(content_buf.c_str(), sz);
				//获取字节数计算content位置
				if (outfile.tellp() == content_length) {//获取当前文件流的位置
					//cout << "结束" << endl;
					
					try {

						std::cout << "进行了" << iocount << "次tcpio" << endl;
						//通知对方yi接收文件
						//auto times = endtime - starttime;
						//cout << times.count() << endl;

					}
					catch (std::exception e) {
						std::cout << e.what() << endl;
					}
					sock_->shutdown(boost::asio::socket_base::shutdown_receive, er);
					stop();
					exit(1);
					return;
				}
				else if (outfile.tellp() > content_length) {
					sock_->shutdown(boost::asio::socket_base::shutdown_receive, er);
					stop();
					exit(0);
					return;
				}

				
				auto endtime_seconds = std::chrono::system_clock::now();
				content_size += sz;
				auto duration_seconds = std::chrono::duration_cast<std::chrono::microseconds>(endtime_seconds - starttime_seconds);
				if (duration_seconds.count()>=1000000) {
					std::cout << "下载速度为" << ((double)(((double)content_size) / 1024.0 / 1024.0)) / (((double)duration_seconds.count()) / 1000.0 / 1000.0) << "mb/s" << endl;
					starttime_seconds= std::chrono::system_clock::now();
					content_size = 0;
				}
				//cout << "当前流的大小" << outfile.tellp() << endl;
				//cout << "一次读入了几个字节" << sz << endl;

			}
			do_content_recv();
		}
		});
}

void fclint::ls_file() {
	Fhead fh;
	fh.type = 'l';
	//fh.acc = this->acc;
	//fh.sendto = this->sendto;
	fh.acc = 10086;
	fh.sendto = 10086;

	fh.content_length = 0;
	//fh.sizeoffilename = filename.size();
	string headbuf((char*)&fh, sizeofFhead);
	sock_->write_some(buffer(headbuf));
	std::cout << "请求获取文件列表中" << endl;
	string tmpbuf;
	tmpbuf.clear();
	tmpbuf.resize(1024);
	sock_->read_some(buffer(tmpbuf));
#ifdef UNIX_SERVER
	tmpbuf = CharConvert::UTF8ToGBK(tmpbuf.c_str());
#endif
	std::cout <<"文件列表:"<<endl<< tmpbuf << endl;
	exit(1);
}
void fclint::stop() {
	sock_->close();
	service.stop();
	if (infile.is_open()) {
		infile.close();
	}
	if (outfile.is_open()) {
		//string s;
		//s += to_string(outfile.tellp());
		//outfile.write(s.c_str(), s.size());

		outfile.close();
	}
}