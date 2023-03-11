#include "mysql_pool.h"

#include<iostream>
MysqlPool* MysqlPool::mysqlpool_object = NULL;
std::mutex MysqlPool::objectlock;
std::mutex MysqlPool::poollock;

MysqlPool::MysqlPool() {}

/*
 *�������ݿ����
 */
void MysqlPool::setParameter(const char* mysqlhost,
    const char* mysqluser,
    const char* mysqlpwd,
    const char* databasename,
    unsigned int  port,
    const char* socket,
    unsigned long client_flag,
    unsigned int  max_connect) {
    _mysqlhost = mysqlhost;
    _mysqluser = mysqluser;
    _mysqlpwd = mysqlpwd;
    _databasename = databasename;
    _port = port;
    _socket = socket;
    _client_flag = client_flag;
    MAX_CONNECT = max_connect;
}

/*
 *�вεĵ������������ڵ�һ�λ�ȡ���ӳض��󣬳�ʼ�����ݿ���Ϣ��
 */
MysqlPool* MysqlPool::getMysqlPoolObject() {
    if (mysqlpool_object == NULL) {
        objectlock.lock();
        if (mysqlpool_object == NULL) {
            mysqlpool_object = new MysqlPool();
        }
        objectlock.unlock();
    }
    return mysqlpool_object;
}

/*
 *����һ�����Ӷ���
 */
MYSQL* MysqlPool::createOneConnect() {
    MYSQL* conn = NULL;
    conn = mysql_init(conn);
    if (conn != NULL) {
        if (mysql_real_connect(conn,
            _mysqlhost,
            _mysqluser,
            _mysqlpwd,
            _databasename,
            _port,
            _socket,
            _client_flag)) {
            connect_count++;
            return conn;
        }
        else {
            std::cout << mysql_error(conn) << std::endl;
            return NULL;
        }
    }
    else {
        std::cerr << "init failed" << std::endl;
        return NULL;
    }
}

/*
 *�жϵ�ǰMySQL���ӳص��Ƿ��
 */
bool MysqlPool::isEmpty() {
    return mysqlpool.empty();
}
/*
 *��ȡ��ǰ���ӳض��еĶ�ͷ
 */
MYSQL* MysqlPool::poolFront() {
    return mysqlpool.front();
}
/*
 *
 */
unsigned int MysqlPool::poolSize() {
    return mysqlpool.size();
}
/*
 *������ǰ���ӳض��еĶ�ͷ
 */
void MysqlPool::poolPop() {
    mysqlpool.pop();
}
/*
 *��ȡ���Ӷ���������ӳ��������ӣ���ȡ��;û�У������´���һ�����Ӷ���
 *ͬʱע�⵽MySQL�����ӵ�ʱЧ�ԣ��������Ӷ�����,���Ӷ����ڳ���һ����ʱ���û�н��в�����
 *MySQL���Զ��ر����ӣ���Ȼ��������ԭ�򣬱��磺���粻�ȶ��������������жϡ�
 *�����ڻ�ȡ���Ӷ���ǰ����Ҫ���ж����ӳ������Ӷ����Ƿ���Ч��
 *���ǵ����ݿ�ͬʱ�������������������ƣ��ڴ�������������ǰ�жϵ�ǰ�������������������趨ֵ��
 */
MYSQL* MysqlPool::getOneConnect() {
    poollock.lock();
    MYSQL* conn = NULL;
    if (!isEmpty()) {
        while (!isEmpty() && mysql_ping(poolFront())) {
            mysql_close(poolFront());
            poolPop();
            connect_count--;
        }
        if (!isEmpty()) {
            conn = poolFront();
            poolPop();
        }
        else {
            if (connect_count < MAX_CONNECT)
                conn = createOneConnect();
            else
                std::cerr << "the number of mysql connections is too much!" << std::endl;
        }
    }
    else {
        if (connect_count < MAX_CONNECT)
            conn = createOneConnect();
        else
            std::cerr << "the number of mysql connections is too much!" << std::endl;
    }
    poollock.unlock();
    return conn;
}
/*
 *����Ч�����Ӷ���Ż����ӳض����У��Դ��´ε�ȡ�á�
 */
void MysqlPool::close(MYSQL* conn) {
    if (conn != NULL) {
        poollock.lock();
        mysqlpool.push(conn);
        poollock.unlock();
    }
}
/*
 * sql���ִ�к����������ؽ����û�н����SQL��䷵�ؿս����
 * ÿ��ִ��SQL��䶼����ȥ���Ӷ�����ȥһ�����Ӷ���
 * ִ����SQL��䣬�Ͱ����Ӷ���Ż����ӳض����С�
 * ���ض�����map��Ҫ���ǣ��û�����ͨ�����ݿ��ֶΣ�ֱ�ӻ�ò�ѯ���֡�
 * ���磺m["�ֶ�"][index]��
 */
std::map<const std::string, std::vector<const char*> >  MysqlPool::executeSql(const char* sql) {
    MYSQL* conn = getOneConnect();
    std::map<const std::string, std::vector<const char*> > results;
    if (conn) {
        if (mysql_query(conn, sql) == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                MYSQL_FIELD* field;
                while ((field = mysql_fetch_field(res))) {
                    results.insert(make_pair(field->name, std::vector<const char*>()));
                    std::cout << "field name:" << field->name << " ";
                }
                std::cout<<std::endl;
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    unsigned int i = 0;
                    for (std::map<const std::string, std::vector<const char*> >::iterator it = results.begin();
                        it != results.end(); ++it) {
                        (it->second).push_back(row[i++]);
                    }
                }
                mysql_free_result(res);
            }
            else {
                if (mysql_field_count(conn) != 0)
                    std::cerr << mysql_error(conn) << std::endl;
            }
        }
        else {
            std::cerr << mysql_error(conn) << std::endl;
        }
        close(conn);
    }
    else {
        std::cerr << mysql_error(conn) << std::endl;
    }
    return results;
}
/*
 * ���������������ӳض����е�����ȫ���ر�
 */
MysqlPool::~MysqlPool() {
    while (poolSize() != 0) {
        mysql_close(poolFront());
        poolPop();
        connect_count--;
    }
}

#if 0
#include"include/mysqlpool.h"

int main() {
    MysqlPool* mysql = MysqlPool::getMysqlPoolObject();
    mysql->setParameter("localhost", "root", "root", "mysqltest", 0, NULL, 0, 2);
    std::map<const std::string, std::vector<const char*> > m = mysql->executeSql("select * from test");
    for (std::map<const std::string, std::vector<const char*> >::iterator it = m.begin(); it != m.end(); ++it) {
        std::cout << it->first << std::endl;
        const std::string field = it->first;
        for (size_t i = 0; i < m[field].size(); i++) {
            std::cout << m[field][i] << std::endl;
        }
    }
    delete mysql;
    return 0;
}
#endif