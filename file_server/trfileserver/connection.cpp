#define DEBUG
#ifdef DEBUG
#include <iostream>
#endif

#include "connection.hpp"
#include <utility>
#include <vector>
#include "connection_manager.hpp"
//#include "request_handler.hpp"

namespace http {
    namespace server {

        connection::connection(boost::asio::ip::tcp::socket socket,
            connection_manager& manager, int& maxSizeOnce)
            : socket_(std::move(socket)),
            connection_manager_(manager)
        {
			ONCE_READ = 65536 * 16 * maxSizeOnce;
        }

        void connection::start()
        {
            do_read();//start����do_read
        }

        void connection::stop()
        {
            socket_.close();
        }

#ifdef FILETR
		void connection::do_read() {
			headbuf.clear();
			headbuf.resize(1024);
			auto self = shared_from_this();
			async_read(socket_, buffer(headbuf), transfer_exactly(sizeofFFhead), [this, self](boost::system::error_code er, size_t sz) {
				//async_read(socket_, buffer(headbuf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
				if (!er) {
					Fhead* h = (Fhead*)headbuf.c_str();
					if (h->type == 'r') {//
						try {
								sendto = 10086;
								acc = 10086;
								content_length = h->content_length;
								filename = string(h->filename);
								if (content_length > 0 && filename.size() > 0) {
									async_write(socket_, buffer("ok"), [this, self](boost::system::error_code er, size_t sz) {
										if (!er) {
											do_content_read();
										}
										else if (er != boost::asio::error::operation_aborted)//�þ���˼����������׽���δ�رվ͹ر��׽���
										{
											connection_manager_.stop(shared_from_this());
										}
										});
								}
								else {
									cout << __LINE__ << "Զ���ļ�����" << endl;
									connection_manager_.stop(shared_from_this());//�ر��׽���
								}	
						}
						catch (std::exception e) {
							cout << __LINE__ << e.what() << endl;
							connection_manager_.stop(shared_from_this());//�ر��׽���
							return;
						}
					}
					else if (h->type == 's') {
						//cout << "��ǰ���ļ����߳�" << this_thread::get_id();
						//���ļ����ж��Ƿ��Ƕϵ��ļ� .trtmp
						string f(h->filename);
						int pos = 0;
						if ((pos=f.rfind(".trtmp")) != string::npos) {
							f=f.substr(0,pos);
							cur_offset = h->content_length;
						}
						path = workPath + f;
						infile.open(path, std::ios::in | std::ios::binary);
						if (!infile.is_open()) {
							cout << "file is open error" << endl;
							connection_manager_.stop(shared_from_this());
							return;
						}
						try {
							//cout << ONCE_READ << endl;
							infile.seekg(0,std::ios::end);//��ȡ�ļ���С
							content_length = infile.tellg();
							content_length -= cur_offset;
							infile.seekg(cur_offset,std::ios::beg);
							std::array<char, BUFSIZE>buf;
							cout << "�ļ�����" << content_length << endl;
							//char* buf = (char*)malloc(BUFSIZE);
							int len = 0;
							contentbuf.resize(ONCE_READ+BUFSIZE);//���õı�ONCE_READ���BUFSIZ���У���Ϊÿ��Ҫ���ļ���ȡBUFSIZE��
							contentbuf.clear();//�ǵü���䣬��buffer��һ��
							while (1) {
								len = infile.read(buf.data(), BUFSIZE).gcount();
								curFileSize += len;
								//len = infile.read(buf, BUFSIZE).gcount();
								if (len <= 0)break;
								contentbuf.append(string(buf.data(), len));
								if (curFileSize >= ONCE_READ) {
									//���curFilesize�Ѿ�����buf�����˾��˳�����֮����Ժ��ٶ�
									break;
								}
							}

							infile.close();
							Fhead fh;
							//sendto = h->sendto;
							//acc = h->acc;
							sendto = 10086;
							acc = 10086;
							filename = string(h->filename);
							fh.type = 's';
							fh.sendto = h->sendto;//��д�ļ������߸�֪�Է����շ��ѽ����ļ� ������˵�����շ��Ѿ����ո��ļ�
							fh.acc = h->acc;//���շ�
							fh.content_length = content_length;
							strncpy(fh.filename, h->filename, strlen(h->filename));
							headbuf = string((char*)&fh, sizeofFFhead);
						}
						catch (std::exception e) {
							cout << e.what();
							infile.close();
							connection_manager_.stop(shared_from_this());
							return;
						}
						//cout << ((Fhead*)(headbuf.c_str()))->content_length << endl;
						async_write(socket_, buffer(headbuf), [this, self](boost::system::error_code er, size_t sz) {
							if (!er) {
								do_content_write();
								//cout << "do_content_write" << endl;
							}
							else if (er != boost::asio::error::operation_aborted)
							{
								connection_manager_.stop(shared_from_this());
							}
							});
					}
					else if (h->type == 'l') {//��Ӳ鿴ĳĿ¼���ļ����� ���contentlength������0 ����filename��ľ���·��
						
						ls_file();
					}
					else if (h->type == 'd') {//�ڷ������˴����ļ���
#ifdef UNIX
					string path = workPath + string(h->filename);//"./FileRecv/"+2284112699/ 
						int ret=mkdir(h->filename, S_IRWXU);
						if (ret == -1) {
							cout << "dir creat failed" << endl;
							connection_manager_.stop(shared_from_this());
							return;
						}
#endif

					}


				}
				else if (er != boost::asio::error::operation_aborted)
				{
					cout <<__LINE__<<" " << ":" << er.what() << endl;
					connection_manager_.stop(shared_from_this());
				}
			});
		}
		void connection::do_content_write() {
			async_write(socket_, buffer(contentbuf, contentbuf.size()), [this](boost::system::error_code er, std::size_t) {
				bool iscontinue_write(false);
				if (!er)
				{
					if (curFileSize == content_length) {
						// Initiate graceful connection closure.
						boost::system::error_code ignored_ec;//���д���˲�����ͨ���Ȱ�ر�
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
							ignored_ec);
					}
					else {//��û�д���
						//cout << "�Ѿ�����" << curFileSize << "�ֽ�" << endl;
						infile.open(path, std::ios::in | std::ios::binary);
						if (!infile.is_open()) {
							cout << "file transfer has some error" << endl;
							connection_manager_.stop(shared_from_this());
							return;
						}
						infile.seekg(curFileSize, std::ios::beg);
						int len = 0;
						std::array<char, BUFSIZE>buf;
						contentbuf.clear();//�ǵü���䣬��buffer��һ��
						//cout <<"contentbuf_capcity:" << contentbuf.capacity() << endl;
						while (1) {
							len = infile.read(buf.data(), BUFSIZE).gcount();
							if (len <= 0)break;
							contentbuf.append(string(buf.data(), len));
							curFileSize += len;
							if (curFileSize >= ONCE_READ) {
								break;//���curFilesize�Ѿ�����buf�����˾��˳�����֮����Ժ��ٶ�
							}
						}
						//cout <<"��ǰ�ļ�����λ��" << curFileSize << endl;
						infile.close();
						iscontinue_write = true;
						do_content_write();
					}
				}
				if (er != boost::asio::error::operation_aborted)
				{
					if (iscontinue_write)return;
					//cout << __LINE__<<er.what() << endl;
					connection_manager_.stop(shared_from_this());
				}
			});
		}
		void connection::do_FileRecv_write() {
			async_write(socket_, buffer(contentbuf, contentbuf.size()), [this](boost::system::error_code er, std::size_t) {
				bool iscontinue_write(false);
				if (!er)
				{
						// Initiate graceful connection closure.
						boost::system::error_code ignored_ec;//���д���˲�����ͨ���Ȱ�ر�
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
							ignored_ec);
				}
				if (er != boost::asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
				});
		}
		void connection::do_content_read() {
			if (init) {
				//���ļ�������
				char tmp[1024];
				unsigned seed;  // Random generator seed
				// ��ȡ��ǰʱ��
				auto now = std::chrono::system_clock::now();
				// ����1970-01-01 00:00:00��������
				std::chrono::nanoseconds d = now.time_since_epoch();
				// ת��Ϊ������, ���о�����ʧ
				auto millsec = std::chrono::duration_cast<std::chrono::milliseconds>(d);
				snprintf(tmp, sizeof(tmp), "./FileRecv/F_%ld_", millsec.count());
				filename.insert(0, string(tmp));
				outfile.open(filename, ios::out | ios::binary | ios::trunc);
				init = false;
				if (outfile.is_open()) {
					//cout << "��ʼ������" << endl;
				}
				else {
					socket_.close();
					//delete_me();
				}
				starttime = std::chrono::system_clock::now();
				//contentbuf.resize(65536);//���ⲻ�б���ÿ������ָ����С
			}
			//contentbuf.clear();
			contentbuf.resize(65536);
			auto self(shared_from_this());
			async_read(socket_, buffer(contentbuf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
				if (!er) {
					iocount++;
					//cout << "����һ���첽��ȡ����" << endl;
					if (outfile.is_open()) {
						outfile.write(contentbuf.c_str(), sz);
						//��ȡ�ֽ�������contentλ��
						if (outfile.tellp() == content_length) {//��ȡ��ǰ�ļ�����λ��
							auto endtime = std::chrono::system_clock::now();
							try {
								auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endtime - starttime);
								cout << "�����ٶ�Ϊ" << ((double)(((double)content_length) / 1024.0 / 1024.0)) / (((double)duration.count()) / 1000.0 / 1000.0) << "mb/s" << endl;
							}
							catch (std::exception e) {

							}
							outfile.close();

							//delete_me();
							return;
						}
						else if (outfile.tellp() > content_length) {
							try {
								//error_handle();
								//delete_me();
								connection_manager_.stop(shared_from_this());
								outfile.close();
							}
							catch (...) {

							}
							return;
						}

					}
					do_content_read();
				}
				else if (er != boost::asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
				});
		}
		void connection::ls_file() {
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
				cout <<__LINE__<< e.what() << endl;
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
				handle = _findfirst("./FileRecv/*", &fileinfo);
				do
				{
					//�ҵ����ļ����ļ���
					if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
					contentbuf += string(fileinfo.name);
					contentbuf.append("\n");
					//printf("%s\n", fileinfo.name);

				} while (!_findnext(handle, &fileinfo));
				_findclose(handle);
			}
			catch (...) {
				contentbuf += "cat file error\n";
				do_FileRecv_write();
			}
			do_FileRecv_write();
#endif //WIN
		}
#endif //FILETR


    } // namespace server
} // namespace http