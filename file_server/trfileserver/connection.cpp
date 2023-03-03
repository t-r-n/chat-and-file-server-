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
            do_read();//start调用do_read
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
										else if (er != boost::asio::error::operation_aborted)//该句意思是如果本端套接字未关闭就关闭套接字
										{
											connection_manager_.stop(shared_from_this());
										}
										});
								}
								else {
									cout << __LINE__ << "远端文件有误" << endl;
									connection_manager_.stop(shared_from_this());//关闭套接字
								}	
						}
						catch (std::exception e) {
							cout << __LINE__ << e.what() << endl;
							connection_manager_.stop(shared_from_this());//关闭套接字
							return;
						}
					}
					else if (h->type == 's') {
						//cout << "当前发文件的线程" << this_thread::get_id();
						//从文件名判断是否是断点文件 .trtmp
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
							infile.seekg(0,std::ios::end);//获取文件大小
							content_length = infile.tellg();
							content_length -= cur_offset;
							infile.seekg(cur_offset,std::ios::beg);
							std::array<char, BUFSIZE>buf;
							cout << "文件长度" << content_length << endl;
							//char* buf = (char*)malloc(BUFSIZE);
							int len = 0;
							contentbuf.resize(ONCE_READ+BUFSIZE);//设置的比ONCE_READ大出BUFSIZ就行，因为每次要从文件读取BUFSIZE，
							contentbuf.clear();//记得加这句，把buffer清一清
							while (1) {
								len = infile.read(buf.data(), BUFSIZE).gcount();
								curFileSize += len;
								//len = infile.read(buf, BUFSIZE).gcount();
								if (len <= 0)break;
								contentbuf.append(string(buf.data(), len));
								if (curFileSize >= ONCE_READ) {
									//如果curFilesize已经等于buf容量了就退出来，之后的以后再读
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
							fh.sendto = h->sendto;//填写文件发送者告知对方接收方已接收文件 ，到这说明接收方已经接收该文件
							fh.acc = h->acc;//接收方
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
					else if (h->type == 'l') {//添加查看某目录下文件功能 如果contentlength不等于0 看看filename里的具体路径
						
						ls_file();
					}
					else if (h->type == 'd') {//在服务器端创建文件夹
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
						boost::system::error_code ignored_ec;//如果写完了不会在通信先半关闭
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
							ignored_ec);
					}
					else {//还没有传完
						//cout << "已经传输" << curFileSize << "字节" << endl;
						infile.open(path, std::ios::in | std::ios::binary);
						if (!infile.is_open()) {
							cout << "file transfer has some error" << endl;
							connection_manager_.stop(shared_from_this());
							return;
						}
						infile.seekg(curFileSize, std::ios::beg);
						int len = 0;
						std::array<char, BUFSIZE>buf;
						contentbuf.clear();//记得加这句，把buffer清一清
						//cout <<"contentbuf_capcity:" << contentbuf.capacity() << endl;
						while (1) {
							len = infile.read(buf.data(), BUFSIZE).gcount();
							if (len <= 0)break;
							contentbuf.append(string(buf.data(), len));
							curFileSize += len;
							if (curFileSize >= ONCE_READ) {
								break;//如果curFilesize已经等于buf容量了就退出来，之后的以后再读
							}
						}
						//cout <<"当前文件流的位置" << curFileSize << endl;
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
						boost::system::error_code ignored_ec;//如果写完了不会在通信先半关闭
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
				//打开文件描述符
				char tmp[1024];
				unsigned seed;  // Random generator seed
				// 获取当前时间
				auto now = std::chrono::system_clock::now();
				// 距离1970-01-01 00:00:00的纳秒数
				std::chrono::nanoseconds d = now.time_since_epoch();
				// 转换为毫秒数, 会有精度损失
				auto millsec = std::chrono::duration_cast<std::chrono::milliseconds>(d);
				snprintf(tmp, sizeof(tmp), "./FileRecv/F_%ld_", millsec.count());
				filename.insert(0, string(tmp));
				outfile.open(filename, ios::out | ios::binary | ios::trunc);
				init = false;
				if (outfile.is_open()) {
					//cout << "初始化结束" << endl;
				}
				else {
					socket_.close();
					//delete_me();
				}
				starttime = std::chrono::system_clock::now();
				//contentbuf.resize(65536);//放这不行必须每次重新指定大小
			}
			//contentbuf.clear();
			contentbuf.resize(65536);
			auto self(shared_from_this());
			async_read(socket_, buffer(contentbuf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
				if (!er) {
					iocount++;
					//cout << "进行一次异步读取任务" << endl;
					if (outfile.is_open()) {
						outfile.write(contentbuf.c_str(), sz);
						//获取字节数计算content位置
						if (outfile.tellp() == content_length) {//获取当前文件流的位置
							auto endtime = std::chrono::system_clock::now();
							try {
								auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endtime - starttime);
								cout << "下载速度为" << ((double)(((double)content_length) / 1024.0 / 1024.0)) / (((double)duration.count()) / 1000.0 / 1000.0) << "mb/s" << endl;
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
				//第一次查找
				handle = _findfirst("./FileRecv/*", &fileinfo);
				do
				{
					//找到的文件的文件名
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