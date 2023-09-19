#ifndef HOST_HPP
#define HOST_HPP

#include <string>
#include <map>

#include "Server.hpp"
#include "Location.hpp"
#include "Util.hpp"

/*
server {
	listen 80; // default: 80
  server_name localhost; // default: ""

	root ./html; // default: /usr/share/nginx/html or /etc/nginx/html

	client_max_body_size 1m; // default: 1m;
	
	error_page 404 /404.html; // default: 엔진엑스 기본 출력 페이지

	location {
		limit_except GET POST; // default: 모두 허용
		autoindex on; // default: off
		index index.html index.htm; //default index.html
		return 301 /new-path/; 
	}
}
*/

const std::string DEFAULT_HOST_NAME = std::string("");
const std::string DEFAULT_ROOT = std::string("/usr/share/nginx/html");
const unsigned int DEFAULT_CLIENT_MAX_BODY_SIZE = 1000000;

class Server;
class Location;

class Host
{

private:
  Server&                                 _server;
  std::string                             _hostname;          // identity
  std::string                             _root;
  unsigned int                            _clientMaxBodySize;
  std::map<int, std::string>              _errorPage;
  std::vector<Location>                   _locations;
  // std::vector<Location*>                  _locations;

public:
  Host(
    const Server &server,
    const std::string &hostname = DEFAULT_HOST_NAME,
    const std::string &root = DEFAULT_ROOT,
    unsigned int clientMaxBodySize = DEFAULT_CLIENT_MAX_BODY_SIZE
  );
  virtual ~Host(void);

private:
  void initLocation(const std::string &url);

public:

  bool isClientMaxBodySizeExceeded(unsigned int size) const;
  bool isMatched(const std::string &hostname) const;
  std::string getErrorPage(int statusCode) const;
  const Location& matchLocation(const std::string &uri) const;


public:
  // 게터 세터
  const Server &getServer(void) const;
  std::string getHostname(void) const;
  const std::vector<Location> getLocations(void) const;
};

std::ostream &operator<<(std::ostream &os, const Host &host);
#endif