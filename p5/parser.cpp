#include "parser.h"

Parser::Parser()
{
    cmds = std::vector<Entity *>();
}

void
Parser::parse(
        std::string instructs)
{
     clear();
     bkgr_mode = false;
     // заменяем вхождения || и && на символы @ и $
     replace(instructs, std::string("||"), std::string("@"));
     replace(instructs, std::string("&&"), std::string("$"));

     // выделяем команды
     std::string symcmds("|@$&;");
     for (size_t pos = 0; pos < instructs.size();)  {
        size_t found = instructs.find_first_of(symcmds, pos);
        if (found != std::string::npos) {
            add_comand(instructs.substr(pos, found - pos));
            add_delim(instructs.substr(found, 1));
            pos = found + 1;
        } else {
            add_comand(instructs.substr(pos, instructs.size() - pos));
            break;
        }
     }
     // перенаправляем stdin/stdout для | и определяем фоновый режим или нет &
     for(auto iter = cmds.begin(); iter != cmds.end(); iter++) {
         if ((*iter)->who_is() == "del" && (dynamic_cast<Delimeter *>(*iter))->delim == "|") {
             dynamic_cast<Comand *>(*(iter - 1))->outfile = "pipe";
             dynamic_cast<Comand *>(*(iter + 1))->infile = "pipe";
         }
     }
     if (cmds.size() > 0 && (*(cmds.end() - 1))->who_is() == "del") {
         if (dynamic_cast<Delimeter *>(*(cmds.end() - 1))->delim == "&") {
             bkgr_mode = true;
             cmds.erase(cmds.end() - 1);
         }
     }
}

void
Parser::replace(
        std::string &str,
        const std::string &fromstr,
        const std::string &tostr)
{
    size_t found;
    for (size_t pos = 0; pos < str.size();) {
        found = str.find(fromstr, pos);
        if (found != std::string::npos) {
            str.replace(found, fromstr.size(), tostr);
            pos = found + tostr.size();
        } else {
            break;
        }
    }

}

void
Parser::add_comand(
        const std::string &str)
{
    std::vector<std::string> parts = std::vector<std::string>();
    std::string wsdelim("\t\v\r\n\f<> ");
    for(size_t pos = 0;;) {
        size_t found = str.find_first_of(wsdelim, pos);
        if (found == std::string::npos) {
            if (str.size() != pos) {
                parts.push_back(str.substr(pos, str.size()-pos));
            }
            break;
        }
        if (found != pos) {
            parts.push_back(str.substr(pos, found-pos));
        }
        if (str[found] == '<' || str[found] == '>') {
            parts.push_back(str.substr(found, 1));
        }
        pos = found + 1;
    }
    std::string infile;
    std::string outfile;
    for(auto iter = parts.begin(); iter != parts.end(); iter++) {
        if (*iter == std::string("<") || (*iter == std::string(">")) ) {
            if (*iter == std::string("<"))
                infile = *(iter + 1);
            else
                outfile = *(iter + 1);
            auto prev_iter = iter - 1;
            parts.erase(iter);
            iter = prev_iter + 1;
            parts.erase(iter);
            iter = prev_iter;
        }
    }

    cmds.push_back(new Comand(parts, infile, outfile));
}

void
Parser::add_delim(
        const std::string &str)
{
    // заменяем обратно @ на  || и $ на  &&
    if (str == "@")
        cmds.push_back(new Delimeter(std::string("||")));
    else if (str == "$")
        cmds.push_back(new Delimeter(std::string("&&")));
    else
        cmds.push_back(new Delimeter(str));
}

void
Parser::clear()
{
    for(auto iter = cmds.begin(); iter != cmds.end(); iter++) {
        delete (*iter);
    }
    cmds.clear();
}

const std::vector<Entity *> &
Parser::get_cmds()
{
    return cmds;
}

bool
Parser::have_bkgr_mode()
{
    return bkgr_mode;
}


