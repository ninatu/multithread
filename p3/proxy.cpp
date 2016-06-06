#include "proxy.h"

void handle_accept(client_server_prt clientserver, const boost::system::error_code & error) {
    if (error) {
        clientserver->set_new_client();
        clientserver->acceptor.async_accept(clientserver->client()->socket(),
                          boost::bind(handle_accept, clientserver, _1));
    } else {
        clientserver->client()->start();
        clientserver->add_client();
        clientserver->set_new_client();
        clientserver->acceptor.async_accept(clientserver->client()->socket(),
                      boost::bind(handle_accept, clientserver, _1));
    }
}

Proxy::Proxy(boost::asio::io_service &_service, std::string config) {
    std::istringstream stconfig(config);
    std::vector<std::string> src_config;
    while (1) {
        std::string tmp;
        std::getline(stconfig, tmp, '\n');
        if (tmp.size() == 0)
            break;
        src_config.push_back(tmp);
    }
    for (auto iter_src = src_config.begin(); iter_src != src_config.end(); iter_src++) {
        std::istringstream conf_src(*iter_src);
        std::string src_port;
        vector<std::pair<std::string, std::string>> dsts = vector<std::pair<std::string, std::string>>();
        std::getline(conf_src, src_port, ',');
        while(true) {
            std::string dst_ip;
            std::string dst_port;
            std::getline(conf_src, dst_ip, ':');
            if (dst_ip.size() == 0)
                break;
            std::getline(conf_src, dst_port, ',');
            dst_port.erase(remove(dst_port.begin(), dst_port.end(), '\n'), dst_port.end());
            dsts.push_back(std::pair<std::string, std::string>(dst_ip, dst_port));
        }
        client_server_prt new_clnt_srvr(new ClientServer(_service, src_port, dsts));
        client_servers.push_back(new_clnt_srvr);
    }
}

ClientServer::ClientServer(boost::asio::io_service &_service, std::string src_port,
                           std::vector<std::pair<std::string, std::string>> &dsts): service(_service),
                           acceptor(_service, ip::tcp::endpoint(ip::tcp::v4(), stoi(src_port)))
{
    servers = dsts;
    std::uniform_int_distribution<int> tmp(0, servers.size() - 1);
    distribution.param(tmp.param());
    set_new_client();
    clients = map<uint64_t, client_ptr>();
    acceptor.async_accept(cur_client->socket(), boost::bind(handle_accept, client_server_prt(this), _1));
}

void ClientServer::set_new_client()
{
    int n_server = distribution(generator);
    cur_client = client_ptr(new Client(ind_number, *this, service,
                            servers[n_server].first, servers[n_server].second));
    ind_number++;
}
void ClientServer::add_client()
{
    clients[ind_number] = cur_client;
}

client_ptr ClientServer::client() { return cur_client; }

void ClientServer::stop(uint64_t number) {
    clients.erase(number);
}

Client::Client(uint64_t _ind_number, ClientServer &_father, io_service &_service,
               string dst_ip, string dst_port) :father(_father), ind_number(_ind_number),
               user_sock(_service), server_sock(_service)

{
    server_addr =  ip::address::from_string(dst_ip);
    server_port = stoi(dst_port);
}

ip::tcp::socket &Client::socket() { return user_sock; }

void Client::start() {
    server_sock.async_connect(ip::tcp::endpoint(server_addr, server_port),
                            boost::bind(&Client::connect,
                            shared_from_this(), _1));
    std::cout << "Client started." << endl;
}
void Client::stop() {
    server_sock.close();
    user_sock.close();
    father.stop(ind_number);
    cout << "Client stop." << endl;

}


void Client::connect(const boost::system::error_code error)
{
    if (error) {
        std::cout << "Error connect with server." << endl;
        user_sock.close();
        server_sock.close();
    } else {
        std::cout << "Server is connected." << endl;
        server_work = true;
        serv_cur_size = user_cur_size = 0;
        user_sock.async_receive(buffer(user_buffer, BUF_SIZE),
                           boost::bind(&Client::userRead,
                           shared_from_this(), _1, _2));
        server_sock.async_receive(buffer(server_buffer, BUF_SIZE),
                           boost::bind(&Client::serverRead,
                           shared_from_this(), _1, _2));
    }
}
void Client::userRead(const boost::system::error_code error, size_t size)
{
    if (error) {
        stop();
    } else {
        user_cur_size = 0;
        user_size_message = size;
        if (size != 0) {
            server_sock.async_send(buffer(user_buffer, size),
                               boost::bind(&Client::userWrite,
                               shared_from_this(), _1, _2));
        } else {
            user_sock.async_receive(buffer(user_buffer, BUF_SIZE),
                               boost::bind(&Client::userRead,
                               shared_from_this(), _1, _2));
        }

    }
}

void Client::userWrite(const boost::system::error_code error, size_t size)
{
    if (error) {
        stop();
    } else {
        user_cur_size += size;
        if (user_size_message == user_cur_size) {
            user_size_message = user_cur_size = 0;
            user_sock.async_receive(buffer(user_buffer, BUF_SIZE),
                               boost::bind(&Client::userRead,
                               shared_from_this(), _1, _2));
        } else {
        server_sock.async_send(buffer(user_buffer + user_cur_size,
                                      user_size_message - user_cur_size),
                               boost::bind(&Client::userWrite,
                               shared_from_this(), _1, _2));
        }
    }
}
void Client::serverRead(const boost::system::error_code error, size_t size) {
    if (error) {
        stop();
    } else {
        serv_cur_size = 0;
        serv_size_message = size;
        if (size != 0) {
            user_sock.async_send(buffer(server_buffer, size),
                               boost::bind(&Client::serverWrite,
                               shared_from_this(), _1, _2));
        } else {
            user_sock.async_receive(buffer(server_buffer, BUF_SIZE),
                               boost::bind(&Client::serverRead,
                               shared_from_this(), _1, _2));
        }

    }
}

void Client::serverWrite(const boost::system::error_code error, size_t size) {
    if (error) {
        stop();
    } else {
        serv_cur_size += size;
        if (serv_size_message == serv_cur_size) {
            serv_size_message = serv_cur_size = 0;
            server_sock.async_receive(buffer(server_buffer, BUF_SIZE),
                               boost::bind(&Client::serverRead,
                               shared_from_this(), _1, _2));
        } else {
        user_sock.async_send(buffer(server_buffer + serv_cur_size,
                                      serv_size_message - serv_cur_size),
                               boost::bind(&Client::serverWrite,
                               shared_from_this(), _1, _2));
        }
    }
}

