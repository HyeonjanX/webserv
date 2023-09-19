#include "Location.hpp"
#include "Util.hpp"

Location::Location(
    Host &host,
    const std::string &uri,
    const std::vector<std::string> &limitExcept,
    bool autoindex,
    const std::vector<std::string> &index,
    const std::pair<int, std::string> &redirect
)   :   _host(host),
        _uri(uri),
        _limitExcept(limitExcept),
        _autoindex(autoindex),
        _index(index),
        _redirect(redirect)
{
    // 초기화 로직
}

Location::~Location() { /* 소멸자 로직 */ }

bool Location::isMatched(const std::string &path) const { return Util::startsWith(path, _uri); }

std::string Location::getRedirectUrl(const std::string &path) const
{
    if (_redirect.first == 0)
    {
        return std::string("");
    }
    return _redirect.second + path.substr(_redirect.second.length());
}

int Location::getRedirectStatusCode(void) const { return _redirect.first; }

const Host &Location::getHost(void) const { return _host; }

std::string Location::getUri() const { return _uri; }
unsigned int Location::getUriSize(void) const { return _uri.length(); }

void Location::setLimitExcept(const std::vector<std::string> &limits) { _limitExcept = limits; }
std::vector<std::string> Location::getLimitExcept() const { return _limitExcept; }

void Location::setAutoindex(bool ai) { _autoindex = ai; }
bool Location::getAutoindex() const { return _autoindex; }

void Location::setIndex(const std::vector<std::string> &idx) { _index = idx; }
std::vector<std::string> Location::getIndex() const { return _index; }

void Location::setRedirect(const std::pair<int, std::string> &rd) { _redirect = rd; }
std::pair<int, std::string> Location::getRedirect() const { return _redirect; }

std::ostream &operator<<(std::ostream &os, const Location &location)
{

    os << "   - 로케이션(\"" << location.getUri() << "\")" <<  std::endl;
    return os;
}