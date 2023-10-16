#include "Location.hpp"
#include "Util.hpp"

#define DEBUG_PRINT false

Location::Location(Host &host, const t_location &c):
    _host(host), _uri(c.m_path), _limitExcept(c._limit_except), _autoindex(c._autoindex),
    _index(c._index), _redirect(c._return), _cgiExt(c._cgi), _root(c._root), _clientMaxBodySize(c._client_max_body_size),
    _error_page(c._error_page)
{
    // Nothing to do.
    if (DEBUG_PRINT) std::cout << "Location 생성시 root: " << _root << std::endl;
}

Location::~Location() { /* 소멸자 로직 */ }

bool    Location::isMatched(const std::string &path) const { return Util::startsWith(path, _uri); }

bool    Location::isRedirect() const { return _redirect._status > 0; /* -1이나 0이면 세팅 X*/ }

// _redirect._page: 리다이렉트해야하는 URL
// url: location path (/invalid)
// uri: 양 옆으로 슬래쉬가 붙여진 location path (/invalid/)
// host: localhost:8080
// 반환해야 하는 값 : scheme을 포함한 완전한 redirect URL
std::string Location::getRedirectUrl(const std::string &host, const std::string &url) const
{
	std::string	scheme("http://");
	std::string	redirectPage = _redirect._page;
	std::string	returnPath = scheme + host + url;

	// 오류 케이스까지 동일한 레벨에서 처리
	// redirectPage, url은 문자 비교를 할 예정이므로 비어있어선 안 됨.
    if (_redirect._status == 0 || redirectPage.empty() || url.empty())
    {
		// redirect 되지 않는다면 어떻게 반환해야 할까?
		// 엔진엑스 찾아보기
        return returnPath;
    }
	if (DEBUG_PRINT) std::cout << "PATH, URI, REDIRECT, HOST: " << url << ", " << _uri << ", " << _redirect._page << ", " <<  host << std::endl;

	// 1. 먼저 리다이렉션 페이지가 인터넷 URL인 경우를 처리
	// 올바르지 않은 URL이라면? (//https://) - startswith로 변경해야 할 듯
	if (redirectPage.find("http://") == 0 || redirectPage.find("https://") == 0)
	{
		return redirectPage;
	}

	// 2. 리다이렉션 페이지가 절대 경로인 경우 처리
	// location의 path를 모두 리다이렉트 페이지로 대체한다
	if (redirectPage[0] == '/')
	{
		return scheme + host + redirectPage;
	}

	// 3. 리다이렉션 페이지가 상대 경로인 경우 처리
	// 먼저, url이 파일이냐(/invalid), 디렉터리냐(/invalid/) 에 따른 차이점이 있음
	if (url[url.length() - 1] != '/')
	{
		returnPath = returnPath.substr(0, returnPath.length() - url.length() + 1);
		if (DEBUG_PRINT || true) std::cout << returnPath << std::endl;
	}

	// 무한 루프 생각해보고 엔진엑스와 비교해보기
	// json에서 return value 다시 파악하기

	// 잘못된 경우!
	// 그리고 리다이렉트 페이지가 dotslash로 오는 경우, 제거해야 함
//	if (redirectPage.length() > 2 && redirectPage[0] == '.' && redirectPage[1] == '/')
//	{
//		redirectPage = redirectPage.substr(2);
//	}

	// 위 전처리를 모두 마친 후, 이제 리다이렉션 페이지가 상대 경로인 경우를 처리
	// returnPath는 모두 /로 끝나고, 리다이렉트 페이지는 /로 시작하지 않으니, 이 둘을 붙이기만 하면 된다.
	returnPath = returnPath + redirectPage;
	//std::cout << returnPath << std::endl;

    return returnPath;
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
size_t      Location::getUriSize(void) const { return _uri.length(); }

void        Location::setLimitExcept(const std::vector<std::string> &limits) { _limitExcept = limits; }
const       std::vector<std::string> &Location::getLimitExcept() const { return _limitExcept; }

void        Location::setAutoindex(bool ai) { _autoindex = ai; }
bool        Location::getAutoindex() const { return _autoindex; }

void        Location::setIndex(const std::vector<std::string> &idx) { _index = idx; }
const       std::vector<std::string> &Location::getIndex() const { return _index; }

void        Location::setRedirect(const t_redirect &rd) { _redirect = rd; }
const       t_redirect &Location::getRedirect() const { return _redirect; }

int         Location::getRedirectStatusCode(void) const { return _redirect._status; }
std::string Location::getRedirectPath(void) const { return _redirect._page; }

void        Location::setCgiExt(std::string cgiExt) { _cgiExt = cgiExt; }
std::string Location::getCgiExt(void) const { return _cgiExt; }

void        Location::setRoot(std::string root) { _root = root; }
std::string Location::getRoot(void) const { return _root; }

void        Location::setClientMaxBodySize(size_t value) { _clientMaxBodySize = value; }
size_t      Location::getClientMaxBodySize(void) const { return _clientMaxBodySize; }

const   std::vector<t_status_page>  &Location::getErrorPage() const { return _error_page; }

std::ostream &operator<<(std::ostream &os, const Location &location)
{

    os << "   - 로케이션(\"" << location.getUri() << "\")" <<  std::endl;
    return os;
}
