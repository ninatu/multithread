#ifndef MESSAGESENDER_H
#define MESSAGESENDER_H
#include <map>
#include "messageforsend.h"
class MessageSender
{
public:
    std::map<int, MessageForSend> messanger;
    void add_client(int sock_fd);
    void add_message(const std::string msg);
    void deleteSockFd(int sockfd);

};

#endif // MESSAGESENDER_H
