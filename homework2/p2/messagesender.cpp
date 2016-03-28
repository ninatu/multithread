#include "messagesender.h"

void MessageSender::add_client(int sock_fd)
{
    messanger.emplace(sock_fd, MessageForSend());
}

void MessageSender::add_message(const std::string msg)
{
    for(auto iter = messanger.begin(); iter != messanger.end(); iter++)
    {
        (iter->second).add_message(msg);
    }
}

void MessageSender::deleteSockFd(int sockfd)
{
    auto iter = messanger.find(sockfd);
    if (iter == messanger.end()) {
        messanger.erase(sockfd);
    }

}
