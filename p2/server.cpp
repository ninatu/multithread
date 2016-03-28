#include "server.h"

Server::Server()
{
    masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (masterSocket == -1)  {
        perror("socket");
        exit(1);
    }
    int optval = 1;
    if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(3100);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(masterSocket, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(masterSocket, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll_create");
        exit(1);
    }

    struct epoll_event event;
    event.data.fd = masterSocket;
    event.events = EPOLLIN | EPOLLET; /* edge triggered */
    if (epoll_ctl(epollFd , EPOLL_CTL_ADD, masterSocket, &event) == -1) {
        perror("epoll_ctl");
        exit(1);
    }
    MAX_EVENTS = 128;
    events = (struct epoll_event *) calloc(MAX_EVENTS, sizeof(struct epoll_event));
}
void Server::deleteClient(int clientFd)
{
    if (epoll_ctl(epollFd , EPOLL_CTL_DEL, clientFd, NULL) == -1) {
        perror("epoll_ctl:delete");
        exit(1);
    }
    messangerReceiver.deleteSockFd(clientFd);
    messageSender.deleteSockFd(clientFd);
    shutdown(clientFd, SHUT_RDWR);
    close(clientFd);
    std::cout << "connection terminated" << std::endl;
}
void Server::readClient(int clientFd)
{
    char buf[1024];
    int nbytes ;
    if ((nbytes = recv(clientFd, buf, sizeof(buf), 0)) <= 0) {
        if (nbytes == 0) {
            deleteClient(clientFd);
        } else {
            perror("recv");
            exit(1);
        }
    } else {
        auto msgs = messangerReceiver.process(clientFd, buf, nbytes);
        for(size_t i = 0; i < msgs.size(); i++) {
            messageSender.add_message(msgs[i]);
            std::cout << msgs[i] << std::endl;
        }
        writeMessages();
    }
}

void Server::writeMessages()
{
    for (auto iter = messageSender.messanger.begin(); iter != messageSender.messanger.end(); iter++) {
        int clientFd = iter->first;
        char buf[1024];
        int nbytes ;
        size_t len = (iter->second).getbitarray(buf, 1024);
        while (len != 0) {
            if ((nbytes = send(clientFd, buf, len, 0)) <= 0 ) {
                if (errno & EWOULDBLOCK) {
                    struct epoll_event new_event;
                    new_event.data.fd = clientFd;
                    new_event.events = EPOLLIN | EPOLLET | EPOLLOUT; /* edge triggered */
                    if (epoll_ctl(epollFd , EPOLL_CTL_MOD, clientFd, &new_event) == -1) {
                        perror("epoll_ctl");
                        exit(1);
                    }
                    (iter->second).need_write = true;
                    break;
                } else {
                    perror("recv");
                    exit(1);
                }
            } else {
                (iter->second).erase(nbytes);
                len = (iter->second).getbitarray(buf, 1024);
            }
        }
        if (len == 0 && (iter->second).need_write)  {
            struct epoll_event new_event;
            new_event.data.fd = clientFd;
            new_event.events = EPOLLIN | EPOLLET; /* edge triggered */
            if (epoll_ctl(epollFd , EPOLL_CTL_MOD, clientFd, &new_event) == -1) {
                perror("epoll_ctl");
                exit(1);
            }
            (iter->second).need_write = false;
        }
    }
}

void Server::processMasterEvent(struct epoll_event& event)
{
    if (event.events & EPOLLERR) {
        perror("event_masterSocket");
        exit(1);
    }
    else {       
       struct sockaddr newSockAddr;
       socklen_t addrLen = sizeof(newSockAddr);
       int newFd = accept(masterSocket, (struct sockaddr *)&newSockAddr, &addrLen);
       if (newFd == -1) {
           perror("accept");
           exit(1);
       }
       struct epoll_event new_event;
       new_event.data.fd = newFd;
       new_event.events = EPOLLIN | EPOLLET; /* edge triggered */
       if (epoll_ctl(epollFd , EPOLL_CTL_ADD, newFd, &new_event) == -1) {
           perror("epoll_ctl");
           exit(1);
       }
       messageSender.add_client(newFd);
       messageSender.messanger.find(newFd)->second.add_message(std::string("Welcome"));
       writeMessages();
       std::cout << "accepted connection" << std::endl;
   }

}
void Server::processClientEvent(struct epoll_event& event)
{
    if (event.events & EPOLLERR) {
        perror("event_client");
        exit(1);
    } else if (event.events & EPOLLHUP) {
        deleteClient(event.data.fd);
    } else if (event.events & EPOLLIN) {
       readClient(event.data.fd);
    }

}
void Server::start()
{
    while(true) {
        int n = epoll_wait(epollFd , events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait");
            exit(1);
        }
        for(int i = 0; i < n; i++)
        {
            if (events[i].data.fd == masterSocket) {
                processMasterEvent(events[i]);
            }
            else {
                processClientEvent(events[i]);
            }
        }
    }
}
