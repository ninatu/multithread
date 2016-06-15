#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "comand.h"

class Parser
{
public:
    Parser();
    void parse(std::string);
    const std::vector<Entity *> &get_cmds();
    bool have_bkgr_mode();
private:
    std::vector<Entity *> cmds;
    bool bkgr_mode = false;
    void replace(std::string &str, const std::string &fromstr,
                 const std::string &tostr);
    void add_comand(const std::string &str);
    void add_delim(const std::string &str);
    void clear();


};

#endif // PARSER_H
