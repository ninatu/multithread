#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H
#include <map>
#include "message.h"


class MessageReceiver
{
    std::map<int, Message> messanger;
public:
    std::vector<std::string> process(int sockfd, void *buf, size_t len);
    void deleteSockFd(int sockfd);
};

#endif // MESSAGEHANDLER_H
