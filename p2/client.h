#ifndef CLIENT_H
#define CLIENT_H
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include <fcntl.h>

#include "messagesender.h"
#include "messagereceiver.h"

class Client
{
    int socketFd;
    MessageReceiver messangerReceiver;
    MessageForSend messageForSend;
    fd_set readfds;
    fd_set writefds;
    int fdmax;
    bool needwrite;
    void writeFds(fd_set &write_tmp);
    void readFds(fd_set &read_tmp);
    void readServer();
public:
    Client();
    void start();
};

#endif // CLIENT_H
