#ifndef PROXY_H
#define PROXY_H

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;

#define BUF_SIZE 1024


class ClientServer;
class Proxy;
class Client;
typedef boost::shared_ptr<ClientServer> client_server_prt;
typedef boost::shared_ptr<Client> client_ptr;

void handle_accept(client_server_prt clientserver, const boost::system::error_code & error);

class Proxy {
public:
    Proxy(boost::asio::io_service &_service, std::string config);
private:
    std::vector<client_server_prt> client_servers = std::vector<client_server_prt>();
};



class ClientServer : public boost::enable_shared_from_this<ClientServer>, boost::noncopyable {
    typedef ClientServer self_type;
public:
    typedef boost::system::error_code error_code;
    ClientServer(boost::asio::io_service &_service,
                 std::string src_port, std::vector<std::pair<std::string, std::string>> &dsts);
    void stop(uint64_t number);
    friend void handle_accept(client_server_prt clientserver, const boost::system::error_code & error);

private:
    void set_new_client();
    void add_client();
    client_ptr client();
    boost::asio::io_service &service;
    ip::tcp::acceptor acceptor;
    client_ptr cur_client;
    map<uint64_t, client_ptr> clients;
    uint64_t ind_number = 0;
    std::vector<std::pair<std::string, std::string>> servers;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution;
};

class Client: public boost::enable_shared_from_this<Client>, boost::noncopyable {
public:
    Client(uint64_t _ind_number, ClientServer &_father, boost::asio::io_service &_service,
           std::string dst_ip, std::string dst_port);
    ip::tcp::socket &socket();
    void start();
    void stop();
    void connect(const boost::system::error_code error);
    void userRead(const boost::system::error_code error, size_t size);
    void serverRead(const boost::system::error_code error, size_t size);
    void userWrite(const boost::system::error_code error, size_t size);
    void serverWrite(const boost::system::error_code error, size_t size);

private:
    ClientServer &father;
    uint64_t ind_number;
    ip::tcp::socket user_sock;
    ip::tcp::socket server_sock;
    char user_buffer[BUF_SIZE];
    char server_buffer[BUF_SIZE];
    ip::address server_addr;
    int server_port;
    bool server_work = false;
    bool user_work = false;
    int serv_size_message, user_size_message;
    int serv_cur_size, user_cur_size;
};



#endif // PROXY_H
