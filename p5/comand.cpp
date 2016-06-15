#include "comand.h"

Comand::Comand(
        const std::vector<std::string> &parts,
        const std::string &_infile,
        const std::string &_outfile)
{
    infile = _infile;
    outfile = _outfile;

    cmd = (char*) calloc(parts[0].size() + 1, sizeof(*cmd));
    params = (char **) calloc(parts.size() + 1, sizeof(*params));

    memcpy(cmd, parts[0].c_str(), parts[0].size()+1);
    for(size_t i = 0; i < parts.size(); i++) {
        params[i] = (char*) calloc(parts[i].size() + 1, sizeof(**params));
        memcpy(params[i], parts[i].c_str(), parts[i].size()+1);
    }
    params[parts.size()] = NULL;
    count_params = parts.size();
}

Comand::~Comand()
{
    free(cmd);
    for(size_t i = 0; i < count_params; i++)
        free(params[i]);
    free(params);
}

const std::string &
Comand::who_is()
{
    return name_class;
}

const std::string &
Delimeter::who_is()
{
    return name_class;
}



