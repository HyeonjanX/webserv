#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <map>

#include "Host.hpp"
#include "Config.hpp"

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
    t_redirect                  _redirect;
    std::string                 _cgiExt; // .py .php .js .ts 등
    std::string                 _root;
    size_t                      _clientMaxBodySize;
    std::vector<t_status_page>  _error_page;


public:
    Location(Host &host, const t_location &locationConfig);
    
    virtual ~Location(void);

public:
    bool                                isMatched(const std::string &url) const;
    bool                                isRedirect() const;
    std::string                         getRedirectUrl(const std::string &host, const std::string &url) const;
    bool                                isAllowedMethod(const std::string &method) const;
    

public:
    // getter & setter
    const Host                          &getHost(void) const;
    std::string                         getUri(void) const;
    size_t                              getUriSize(void) const;
    
    void                                setLimitExcept(const std::vector<std::string> &limits);
    const   std::vector<std::string>    &getLimitExcept() const;

    void                                setAutoindex(bool ai);
    bool                                getAutoindex(void) const;

    void                                setIndex(const std::vector<std::string> &idx);
    const   std::vector<std::string>    &getIndex() const;

    void                                setRedirect(const t_redirect &rd);
    const   t_redirect                  &getRedirect() const;

    int                                 getRedirectStatusCode(void) const;
    std::string                         getRedirectPath(void) const;

    void                                setCgiExt(std::string cgiExt);
    std::string                         getCgiExt(void) const;

    void                                setRoot(std::string root);
    std::string                         getRoot(void) const;
    
    void                                setClientMaxBodySize(size_t value);
    size_t                              getClientMaxBodySize(void) const;

    const   std::vector<t_status_page>  &getErrorPage() const;

};

std::ostream &operator<<(std::ostream &os, const Location &location);

#endif
