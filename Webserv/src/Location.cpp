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
    // FOR TEST
    _limitExcept.push_back(std::string("GET"));
    _limitExcept.push_back(std::string("POST"));
}

Location::~Location() { /* 소멸자 로직 */ }

bool    Location::isMatched(const std::string &path) const { return Util::startsWith(path, _uri); }

bool    Location::isRedirect() const { return _redirect.first != 0; }

std::string Location::getRedirectUrl(const std::string &path) const
{
    if (_redirect.first == 0)
    {
        return std::string("");
    }
    return _redirect.second + path.substr(_uri.length());
}

bool    Location::isAllowedMethod(const std::string &method) const
{
    if (_limitExcept.empty())
    {
        return true; // 만약 설정 X => 모두 허용 컨셉
    }
    for (std::vector<std::string>::const_iterator it = _limitExcept.begin();
        it < _limitExcept.end();
        ++it)
    {
        if (it.base()->compare(method) == 0)
        {
            return true;
        }
    }
    return false;
}

const Host &Location::getHost(void) const { return _host; }

std::string Location::getUri() const { return _uri; }
unsigned int Location::getUriSize(void) const { return _uri.length(); }

void Location::setLimitExcept(const std::vector<std::string> &limits) { _limitExcept = limits; }
const std::vector<std::string> &Location::getLimitExcept() const { return _limitExcept; }

void Location::setAutoindex(bool ai) { _autoindex = ai; }
bool Location::getAutoindex() const { return _autoindex; }

void Location::setIndex(const std::vector<std::string> &idx) { _index = idx; }
const std::vector<std::string> &Location::getIndex() const { return _index; }

void Location::setRedirect(const std::pair<int, std::string> &rd) { _redirect = rd; }
const std::pair<int, std::string> &Location::getRedirect() const { return _redirect; }

int Location::getRedirectStatusCode(void) const { return _redirect.first; }
std::string Location::getRedirectPath(void) const { return _redirect.second; }

std::ostream &operator<<(std::ostream &os, const Location &location)
{

    os << "   - 로케이션(\"" << location.getUri() << "\")" <<  std::endl;
    return os;
}