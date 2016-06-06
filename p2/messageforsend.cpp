#include "messageforsend.h"

MessageForSend::MessageForSend()
{
    count = 0;
    beg_offset = 0;
    end_offset = 0;
    need_write = false;
}

void MessageForSend::add_message(const std::string &message)
{
    size_t size_message = message.size();
    char *buf = new char[size_message + 4];
    *((uint32_t *)buf) = size_message;
    memcpy(buf + 4, message.c_str(), size_message);
    size_message += 4;

    size_t offset = 0;
    char *tmp;
    auto end_iter = bitlist.rbegin();
    if (end_iter != bitlist.rend())
    {
        if (end_offset + size_message <= 1024)
        {
            memcpy((*end_iter) + end_offset, buf, size_message);
            offset += size_message;
            end_offset += size_message;
        } else {
            memcpy((*end_iter) + end_offset, buf, 1024 - end_offset);
            offset = 1024 - end_offset;
        }
    }
    while(offset + 1024 <= size_message) {
        tmp = new char[1024];
        memcpy(tmp, buf + offset, 1024);
        offset += 1024;
        bitlist.push_back(tmp);
    }
    if (offset != size_message) {
        tmp = new char[1024];
        memcpy(tmp, buf + offset, size_message - offset);
        end_offset = size_message - offset;
        bitlist.push_back(tmp);
    }
    count += size_message;
    delete []buf;
}

size_t MessageForSend::getbitarray(char *buf, size_t len)
{
    if (len > count)
        len = count;
    if (len == 0)
        return 0;
    auto iter = bitlist.begin();
    size_t offset = 0;
    if (beg_offset + len <= 1024) {
        memcpy(buf, (*iter) + beg_offset, len);
        return len;
    } else {
         memcpy(buf, (*iter) + beg_offset, 1024 - beg_offset);
         offset = 1024 - beg_offset;
         iter ++;
         memcpy(buf + offset, (*iter), len - offset);
         return len;
    }

}

void MessageForSend::erase(size_t len)
{
    if (beg_offset + len < 1024) {
        beg_offset += len;
    } else {
         size_t delta = 1024 - beg_offset;
         char *tmp = bitlist.front();
         delete []tmp;
         bitlist.pop_front();
         beg_offset = len - delta;
    }
    count -=len;
}
