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
	
	//setlocale(LC_ALL, "Chinese-simplified");//�������Ļ���
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
			std::cout << "���ͳɹ�" << endl;
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
	//std::cout << "�����ļ�ͷ" << endl;
	string tmpbuf;
	tmpbuf.clear();
	tmpbuf.resize(1024);
	sock_->read_some(buffer(tmpbuf));
	//std::cout << tmpbuf << endl;
	boost::asio::io_context::work work(service);//����run�˳�
	if (tmpbuf.find("ok") != string::npos) {
		//std::cout << "����do_write" << endl;
		do_write();
	}
	//std::cout << "����" << endl;
	service.run();
	std::cout << "�������" << endl;
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
	//std::cout << "�����ļ�ͷ" << endl;

	tmpbuf.clear();
	tmpbuf.resize(1024);
	//cout << "��ȡ" << sock_->read_some(buffer(tmpbuf)) << "���ֽ�" << endl;
	async_read(*sock_, buffer(tmpbuf, sizeofFhead), transfer_exactly(sizeofFhead), [this](boost::system::error_code er, size_t sz) {
		if (er) {
			sock_->shutdown(boost::asio::socket_base::shutdown_receive, er);
			stop();
			return;
		}
		//std::cout << tmpbuf << endl;
		boost::asio::io_context::work work(service);//����run�˳�
		//char tmp[sizeofFhead];
		//strncpy_s(tmp, tmpbuf.c_str(), sizeofFhead);
		//content_length = ((Fhead*)tmp)->content_length;
		content_length = ((Fhead*)tmpbuf.c_str())->content_length;
		//std::cout <<"�������ݰ����ļ���С" << content_length << endl;
		std::cout << "����do_content_recv" << endl;
		do_content_recv();
		});

	
	//std::cout << "����" << endl;
	service.run();
}
void fclint::do_content_recv() {
	if (init) {
		//���ļ�������
		char tmp[1024];
		unsigned seed;  // Random generator seed
	// Use the time function to get a "seed�� value for srand
		seed = time(0);
		srand(seed);
		// Now generate and print three random numbers
		//cout << rand() << " ";
		snprintf(tmp, sizeof(tmp), "./F_%d", rand() % 10000);
		filename.insert(0, string(tmp));
		outfile.open(filename, ios::out | ios::binary | ios::trunc);
		init = false;
		if (outfile.is_open()) {
			//std::cout << "��ʼ������" << endl;
		}
		
		starttime_seconds = std::chrono::system_clock::now();
		//contentbuf.resize(65536);//���Ż����б���ÿ������ָ����С
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


			//cout << "����һ���첽��ȡ����" << endl;
			if (outfile.is_open()) {
				outfile.write(content_buf.c_str(), sz);
				//��ȡ�ֽ�������contentλ��
				if (outfile.tellp() == content_length) {//��ȡ��ǰ�ļ�����λ��
					//cout << "����" << endl;
					
					try {

						std::cout << "������" << iocount << "��tcpio" << endl;
						//֪ͨ�Է�yi�����ļ�
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
					std::cout << "�����ٶ�Ϊ" << ((double)(((double)content_size) / 1024.0 / 1024.0)) / (((double)duration_seconds.count()) / 1000.0 / 1000.0) << "mb/s" << endl;
					starttime_seconds= std::chrono::system_clock::now();
					content_size = 0;
				}
				//cout << "��ǰ���Ĵ�С" << outfile.tellp() << endl;
				//cout << "һ�ζ����˼����ֽ�" << sz << endl;

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
	std::cout << "�����ȡ�ļ��б���" << endl;
	string tmpbuf;
	tmpbuf.clear();
	tmpbuf.resize(1024);
	sock_->read_some(buffer(tmpbuf));
#ifdef UNIX_SERVER
	tmpbuf = CharConvert::UTF8ToGBK(tmpbuf.c_str());
#endif
	std::cout <<"�ļ��б�:"<<endl<< tmpbuf << endl;
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