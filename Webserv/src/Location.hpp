#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <map>

#include "Host.hpp"

class Server;
class Host;

// location uri { ... }
// limit_except GET POST; // default: 모두 허용
// autoindex on; // default: off
// index index.html index.htm; //default index.html
// rewrite 301 /new-path/;
// 리다이렉트는 return이 아니였음!
class Location
{

private:
    Host&                       _host;
    std::string                 _uri;
    std::vector<std::string>    _limitExcept;
    bool                        _autoindex;
    std::vector<std::string>    _index;
    std::pair<int, std::string> _redirect;

public:
    Location(
        Host &host, const std::string &uri,
        const std::vector<std::string> &limitExcept = std::vector<std::string>(),
        bool autoindex = false,
        const std::vector<std::string> &index = std::vector<std::string>(),
        const std::pair<int, std::string> &redirect = std::make_pair(0, "")
    );
    virtual ~Location(void);

public:
    bool        isMatched(const std::string &url) const;
    std::string getRedirectUrl(const std::string &url) const;
    bool        isAllowedMethod(const std::string &method) const;
    

public:
    // getter & setter
    const Host &getHost(void) const;
    std::string getUri(void) const;
    unsigned int getUriSize(void) const;
    
    void setLimitExcept(const std::vector<std::string> &limits);
    const std::vector<std::string> &getLimitExcept() const;

    void setAutoindex(bool ai);
    bool getAutoindex(void) const;

    void setIndex(const std::vector<std::string> &idx);
    const std::vector<std::string> &getIndex() const;

    void setRedirect(const std::pair<int, std::string> &rd);
    const std::pair<int, std::string> &getRedirect() const;

    int getRedirectStatusCode(void) const;
    std::string getRedirectPath(void) const;
};

std::ostream &operator<<(std::ostream &os, const Location &location);

#endif