#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "proxy.h"
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;

io_service service;

int main(int argc, char* argv[]) {
    if (argc > 2)  {
        cout << "1 parametr - config file" << endl;
        return 0;
    }
    ifstream in_file;
    in_file.open(argv[1]);//open the input file
    stringstream config;
    config << in_file.rdbuf();
    Proxy proxy(service, config.str());
    service.run();
    return 0;
}
