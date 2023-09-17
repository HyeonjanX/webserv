#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <unistd.h>

#include "EventHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Util.hpp"

#include <map>

// #include <vector> at EventHandler.hpp
class Client;

class Webserver
{
private:
  EventHandler _eventHandler;
  std::string _configPath;
  std::map<int, Server> _servers;
  std::map<int, Client> _clients;


public:
  Webserver(int ac, char **av);
  virtual ~Webserver(void);

public:

  void initWebserver(void);
  void runWebserver(void);

private:

  void initServer(int port, int sockreuse = 1, int backlog = 16);
  void initClient(int serverSocket, Server *s = NULL);

  void clientReadProcess(Client &c);

public:
  void closeClient(int clientsocket);
};

#endif