#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <unistd.h>
#include <stdexcept>

#include "EventHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Util.hpp"
#include "Cgi.hpp"

#include "JsonParser.hpp"
#include "Config.hpp"

#include <map>

// #include <vector> at EventHandler.hpp
class Client;

class Webserver
{
private:
    EventHandler                            _eventHandler;
    std::string                             _configPath;
    std::map<int, std::vector<t_host> >     _serverConfigs;
    std::map<int, Server>                   _servers;
    std::map<int, Client>                   _clients;

public:
    Webserver(int ac, char **av);
    virtual ~Webserver(void);

public:
    void initWebserver(void);
    void runWebserver(void);

private:
    void initServer(int port, const std::vector<t_host> &hosts);
    void initClient(int serverSocket, Server *s = NULL);

public:
    void closeClient(int clientsocket);

    std::map<int, Client>::iterator searchClientByPipeFd(int fd);
};

#endif