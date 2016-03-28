#include "message.h"

Message::Message(void *buf, size_t len)
{
    initialise(buf, len);
}
void Message::initialise (void *buf, size_t len)
{
    cur_message = std::string();
    cur_len = 0;
    have_count = false;
    have_part_next_msg = false;
    add_part_msg(buf, len);
}

bool Message::have_full_message()
{
    if (have_count && len_msg == cur_len)
        return true;
    return false;
}

std::vector<std::string> Message::get_message()
{
    std::vector<std::string> msgs;
    while (have_count && len_msg == cur_len) {
        msgs.push_back(cur_message);
        have_count = false;
        cur_len = 0;
        cur_message = std::string();
        if (have_part_next_msg) {
            char tmp_buf[1024];
            memcpy(tmp_buf, add_part, len_add_part);
            initialise(tmp_buf, len_add_part);
        }
    }
    return msgs;
}

void Message::add_to_cur_msg(void *buf, size_t len)
{
    memcpy(cur_part, buf, len);
    cur_len += len;
    cur_part[len] = 0;
    cur_message += cur_part;
}

void Message::add_part_msg(void *buf, size_t len)
{
    if (have_count) {
        if (cur_len + len <= len_msg) {
            add_to_cur_msg(buf, len);
            have_part_next_msg = false;
        } else {
            size_t delta_len = len_msg - cur_len;
            add_to_cur_msg(buf, delta_len);
            buf = ((char *)buf) + delta_len ;
            len = len - delta_len;

            have_part_next_msg = true;
            memcpy(add_part, buf, len);
            len_add_part = len;
        }
    } else {
        memcpy(cur_part + cur_len, buf, len);
        cur_len += len;
        if (cur_len > 4) {
            len_msg =  *((uint32_t*)cur_part);
            have_count = true;
            char tmp_buf[1024];
            size_t tmp_len = cur_len - 4;
            cur_len = 0;
            cur_message = std::string();
            memcpy(tmp_buf, cur_part + 4, tmp_len);
            add_part_msg(tmp_buf, tmp_len);
        }
    }
}
