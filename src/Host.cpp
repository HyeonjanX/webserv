#include "Host.hpp"

Host::Host(
    const Server &server,
    const std::string &hostname,
    const std::string &root,
    unsigned int clientMaxBodySize)
        :   _server(const_cast<Server &>(server)),
            _hostname(hostname),
            _root(root),
            _clientMaxBodySize(clientMaxBodySize) {}

Host::~Host(void) {}

bool Host::isClientMaxBodySizeExceeded(unsigned int size) const { return _clientMaxBodySize < size; }

bool Host::isMatched(const std::string &hostname) const { return Util::caseInsensitiveCompare(hostname, _hostname); }

std::string Host::getErrorPage(int statusCode) const
{
    std::map<int, std::string>::const_iterator it;

    it = _errorPage.find(statusCode);
    return it != _errorPage.end() ? it->second : std::string("");
}

/*
https://www.example.com:8080    
Scheme: https
Authority: www.example.com:8080
Userinfo: 없음
Host: www.example.com
Port: 8080
Path: /path/to/resource
Query: name=example
Fragment: section1
*/
const Location &Host::matchLocation(const std::string &path) const
{
    unsigned int maxUriSize = 0;
    const Location *matched = NULL;

    std::vector<Location>::const_iterator it;

    for (it = _locations.begin(); it != _locations.end(); ++it)
    {
        if (it->isMatched(path) && it->getUriSize() > maxUriSize)
        {
            matched = it.base();
            maxUriSize = it->getUriSize();
        }
    }

    if (matched == NULL)
    {
        throw std::runtime_error("No matching location found"); // 예외 처리 추가
    }

    return *matched;
}

const Server &Host::getServer(void) const { return _server; }
std::string Host::getHostname(void) const { return _hostname; }
