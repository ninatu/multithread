#include "messagereceiver.h"

std::vector<std::string> MessageReceiver::process(int sockfd, void *buf, size_t len) {
    auto iter = messanger.find(sockfd);
    if (iter == messanger.end()) {
        Message new_message(buf, len);
        iter = messanger.emplace(sockfd, new_message).first;
    } else {
        (iter->second).add_part_msg(buf, len);
    }
    return (iter->second).get_message();
}

void MessageReceiver::deleteSockFd(int sockfd)
{
    auto iter = messanger.find(sockfd);
    if (iter == messanger.end()) {
        messanger.erase(sockfd);
    }

}
