#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include "messagereceiver.h"
#include "messagesender.h"
#include <fcntl.h>



class Server
{
    int masterSocket;
    MessageReceiver messangerReceiver;
    MessageSender messageSender;
    int epollFd;
    struct epoll_event * events;
    int count_events;
    int MAX_EVENTS;
    void processMasterEvent(struct epoll_event& event);
    void processClientEvent(struct epoll_event& event);
    void deleteClient(int clientFd);
    void readClient(int clientFd);
    void writeMessages();
public:
    Server();
    void start();
};

#endif // SERVER_H
