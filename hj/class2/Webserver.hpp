#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "EventHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <map>

// #include <vector> at EventHandler.hpp

class Webserver
{
private:
  EventHandler _eventHandler;
  std::string _configPath;
  std::map<int, Server> _servers;
  std::map<int, Server> _clients;
  std::map<int, Server> _files;

public:
  Webserver(int ac, char **av);
  virtual ~Webserver(void);

private:
  void initWebserver(void);

  void initServer(int port, std::string host = "",
                  int sockreuse = 1, int backlog = 16);
  void initClient(int serverSocket);

  void runWebserver(void);

  void clientReadProcess(Client &c);
};
