#ifndef COMAND_H
#define COMAND_H
#include <string>
#include <vector>
#include <cstring>
#include <iostream>

class Entity {
public:
    virtual const std::string &who_is() = 0;
};

class Comand:public Entity
{
public:
    char *cmd;
    char **params;
    size_t count_params;
    std::string infile;
    std::string outfile;
    Comand(const std::vector<std::string> &parts,
           const std::string &_infile,
           const std::string &_outfile);
    ~Comand();
    virtual const std::string &who_is();
private:
    const std::string name_class = std::string("cmd");
};

class Delimeter:public Entity
{
public:
    std::string delim;
    Delimeter(const std::string &_delim):delim(_delim){}
    virtual const std::string &who_is();
private:
    const std::string name_class = std::string("del");
};

#endif // COMAND_H
