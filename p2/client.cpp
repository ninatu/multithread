#include "client.h"

Client::Client()
{
    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == -1)  {
        perror("socket");
        exit(1);
    }
    if (fcntl(socketFd, F_SETFL, fcntl(socketFd, F_GETFL) | O_NONBLOCK) == -1 ) {
        perror("fcntl");
        exit(1);
    }
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(3100);
    sockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(socketFd, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1 && errno != EINPROGRESS) {
        perror("connect");
        exit(1);
    }
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(socketFd, &readfds);
    FD_SET(0, &readfds);
    FD_SET(socketFd, &writefds);
    fdmax = socketFd + 1;
    needwrite = false;
}

void Client::readServer() {
    char buf[1024];
    int nbytes ;
    if ((nbytes = recv(socketFd, buf, sizeof(buf), 0)) <= 0) {
        if (nbytes == 0) {
            //std::cout << "connection terminated" << std::endl;
            shutdown(socketFd, SHUT_RDWR);
            close(socketFd);
            exit(0);
        } else {
            perror("recv");
            exit(1);
        }
    } else {
        auto msgs = messangerReceiver.process(socketFd, buf, nbytes);
        for(size_t i = 0; i < msgs.size(); i++) {
            std::cout << msgs[i] << std::endl;
        }
    }
}

void Client::readFds(fd_set &read_tmp)
{
    if (FD_ISSET(0, &read_tmp)) {
        std::string msg;
        std::cin >> msg;
        messageForSend.add_message(msg);
        needwrite = true;
    }
    if (FD_ISSET(socketFd,  &read_tmp)) {
       readServer();
    }
}

void Client::writeFds(fd_set &write_tmp)
{
    if (FD_ISSET(socketFd,  &write_tmp)) {
        char buf[1024];
        int nbytes ;
        size_t len = messageForSend.getbitarray(buf, 1024);
        while (len != 0) {
            if ((nbytes = send(socketFd, buf, len, 0)) <= 0) {
                if (nbytes == 0) {
                    shutdown(socketFd, SHUT_RDWR);
                    close(socketFd);
                    exit(0);
                } else {
                    if (errno & EWOULDBLOCK) {
                        needwrite = true;
                        break;
                    }
                    else {
                        perror("recv");
                        exit(1);
                    }
                }
            } else {
                messageForSend.erase(nbytes);
                len = messageForSend.getbitarray(buf, 1024);
            }

        }
        if (len == 0)
            needwrite = false;
    }
}

void Client::start()
{
    fd_set read_tmp;
    fd_set write_tmp;
    while(1)
    {
        read_tmp = readfds;
        write_tmp = writefds;
        if (!needwrite) {
            if(select(fdmax, &read_tmp, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(1);
            }
            readFds(read_tmp);

        } else {
            if(select(fdmax, &read_tmp, &write_tmp, NULL, NULL) == -1) {
                perror("select");
                exit(1);
            }
            readFds(read_tmp);
            writeFds(write_tmp);
        }
    }
}
