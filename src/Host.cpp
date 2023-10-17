#include "Host.hpp"
#include <stdexcept>

Host::Host(const Server &server, const t_host &c) :
    _server(const_cast<Server &>(server)), _hostname(c._server_name), _root(c._root),
    _clientMaxBodySize(c._client_max_body_size), _errorPage(c._error_page)
{
    for (size_t i = 0; i < c._locations.size(); ++i)
    {
        _locations.push_back(Location(*this, c._locations[i]));
    }
}

Host::~Host(void) {}

// 게터
const Server                        &Host::getServer(void) const { return _server; }
const std::string                   &Host::getHostname(void) const { return _hostname; }
const std::string                   &Host::getRoot(void) const { return _root; }
size_t                              Host::getClientMaxBodySize(void) const { return _clientMaxBodySize; }
const std::vector<t_status_page>    &Host::getErrorPage(void) const { return _errorPage; }
const std::vector<Location>         &Host::getLocations(void) const { return _locations; }


bool Host::isClientMaxBodySizeExceeded(unsigned int size) const { return _clientMaxBodySize < size; }

bool Host::isMatched(const std::string &hostname) const { return Util::caseInsensitiveCompare(hostname, _hostname); }

std::string Host::getErrorPage(int statusCode) const
{
    for (size_t i = 0; i < _errorPage.size(); ++i)
    {
        for (size_t j = 0; j < _errorPage[i]._status.size(); ++j)
        {
            if (_errorPage[i]._status[j] == statusCode)
                return _errorPage[i]._page;
        }
    }
    return std::string("");
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
const Location*   Host::matchLocation(const std::string &path) const
{
    std::string path2 = path.back() == '/' ? path : path + "/";
    
    unsigned int maxUriSize = 0;
    const Location *matched = NULL;

    std::vector<Location>::const_iterator it;

    for (it = _locations.begin(); it != _locations.end(); ++it)
    {
        if (it->isMatched(path2) && it->getUriSize() > maxUriSize)
        {
            matched = it.base();
            maxUriSize = it->getUriSize();
        }
    }

    // Location / 가 필수라면 아래 상황은 발생하지 않는다. => 
    if (matched == NULL)
    {
        std::cerr << "matchLocation 실패: " << path << std::endl;
        throw 500;
    }

    return matched;
}

std::ostream &operator<<(std::ostream &os, const Host &host)
{
    const std::vector<Location> locations = host.getLocations();

    os << " - 호스트(\"" << host.getHostname() << "\")" <<  std::endl;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        os << locations[i];
    }
    os << std::endl;
    return os;
}