#ifndef MESSAGEFORSEND_H
#define MESSAGEFORSEND_H
#include <list>
#include <string>
#include "string.h"

class MessageForSend
{
    std::list< char *> bitlist;
    size_t count;
    size_t beg_offset;
    size_t end_offset;
public:
    MessageForSend();
    bool need_write = false;
    void add_message(const std::string &message);
    size_t getbitarray(char *buf, size_t len);
    void erase(size_t len);
};

#endif // MESSAGEFORSEND_H
