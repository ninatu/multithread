#ifndef MESSAGE_H
#define MESSAGE_H
#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <iostream>


class Message
{
    bool have_count = false;
    uint32_t len_msg;
    size_t cur_len;
    char cur_part[2048];
    std::string cur_message;

    bool have_part_next_msg = false;
    size_t len_add_part;
    char add_part[2048];

    void add_to_cur_msg(void *buf, size_t len);
    void initialise (void *buf, size_t len);
public:
    Message(void *buf, size_t len);
    bool have_full_message();
    std::vector<std::string> get_message();
    void add_part_msg(void *buf, size_t len);
};

#endif // MESSAGE_H
