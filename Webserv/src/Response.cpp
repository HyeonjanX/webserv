#include "Response.hpp"
#include "Util.hpp"

#define DEBUG_PRINT false

Response::Response(void)
    : _statusCode(0), _totalBytes(0), _sendedBytes(0) {}

Response::~Response(void) {}

void Response::clean(void)
{
    if (DEBUG_PRINT)
    {
        std::cout << "Response::clean 호출: Response를 비워냅니다!" << std::endl;
    }

    _httpVersion.clear();
    _statusCode = 0;

    _headers.clear();
    _body.clear();

    _data.clear();
    _totalBytes = 0;
    _sendedBytes = 0;

    _cookie.clear();
}

/**
 * 1. 요청라인 만들기(httpVersion, statusCode)
 * 2. 헤더필드 만들기(headers)
 * 3. 바디추가하기(body)
 * 4. 1~3의 결과물 _data에 통합하기(oss)
 * 5. sendProcess로 넘어가기전 초기화하기(totalBytes, sendedBytes)
*/
void Response::generateResponseData(void)
{
    const std::string HTTP_VERSION = "HTTP/1.1"; // 우리는 1.1만 지원, 잘못된 HTTP 요청시 대응하기 위함
    const std::string SP(" ");
    const std::string CRLF("\r\n");
    std::ostringstream oss;

    // 1. 요청라인 만들기(httpVersion, statusCode)
    oss << HTTP_VERSION << SP 
        << _statusCode << SP
        << Util::getStatusCodeMessage(_statusCode) << CRLF;

    // 2. 헤더필드 만들기(headers)
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
        oss << it->first << ": " << it->second << CRLF;

    // 3. 쿠키 굽기
    for (std::map<std::string, t_set_cookie>::const_iterator it = _cookie.begin(); it != _cookie.end(); ++it)
        oss << "Set-Cookie: " << it->second.key << "=" << it->second.value << "; httponly; Max-Age=" << it->second.expire  << ";" << CRLF;

    oss << CRLF;

    oss << _body;

    _data = oss.str();
    _totalBytes = _data.size();
    _sendedBytes = 0;

    if (DEBUG_PRINT)
    {
        std::cout << "_body.size(): " << _body.size() << std::endl;
        std::cout << "===========================" << std::endl;
        std::cout << _data << std::endl;
        std::cout << "===========================" << std::endl;
    }
}

void Response::generate100ResponseData(void)
{
    const int STATUS_100_CODE = 100;
    const std::string HTTP_VERSION = "HTTP/1.1"; // 우리는 1.1만 지원, 잘못된 HTTP 요청시 대응하기 위함
    const std::string SP(" ");
    const std::string CRLF("\r\n");
    std::ostringstream oss;

    oss << HTTP_VERSION << SP << STATUS_100_CODE << SP << Util::getStatusCodeMessage(STATUS_100_CODE) << CRLF;
    oss << CRLF;
    _data = oss.str();
    _totalBytes = _data.size();
    _sendedBytes = 0;
}

void Response::setHttpVersion(const std::string &version)
{
    _httpVersion = version;
}

std::string Response::getHttpVersion() const
{
    return _httpVersion;
}

void Response::setStatusCode(int code) { _statusCode = code; }

int Response::getStatusCode() const { return _statusCode; }

void Response::setHeader(const std::string &key, const std::string &value) { _headers[key] = value; }

bool Response::hasHeader(const std::string &key) const { return _headers.find(key) != _headers.end(); }

std::string Response::getHeader(const std::string &key) const
{
    if (_headers.find(key) != _headers.end())
    {
        return _headers.at(key);
    }
    return "";
}

void Response::setBody(const std::string &b) { _body = b; }

std::string Response::getBody(void) const { return _body; }

std::string Response::getData(void) const { return _data; }
// std::string &Response::getData2(void) { return _data; }

size_t Response::getDataLength(void) const { return _data.size(); }

void Response::updateData(size_t bytes) { _data = _data.substr(bytes); }

void Response::updateSendedBytes(size_t bytes) { _sendedBytes += bytes; }

size_t Response::getTotalBytes(void) const { return _totalBytes; }

size_t Response::getSendedBytes(void) const { return _sendedBytes; }

void Response::addCookie(const std::string &key, const std::string &value, size_t expire) { _cookie[key] = t_set_cookie(key, value, expire); }