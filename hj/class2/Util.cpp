#include "Util.hpp"

std::string Util::ltrim(const std::string &str)
{
    std::string::size_type pos = str.find_first_not_of(" \t");
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(pos);
}

std::string Util::rtrim(const std::string &str)
{
    std::string::size_type pos = str.find_last_not_of(" \t");
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(0, pos + 1);
}

std::string Util::lrtrim(const std::string &str)
{
    return ltrim(rtrim(str));
}

std::string Util::getDateString(void)
{
    std::string data;

    // 현재 시간을 time_t 형태로 가져옴
    std::time_t current_time = std::time(0);

    // tm 형태로 변환
    std::tm *time_info = std::localtime(&current_time);

    // 문자열 형태로 변환
    char buffer[80];
    size_t n = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);

    return std::string(buffer, n);
}

void Util::print_kevent_info(const struct kevent &ke)
{
    std::cout << "kevent information:" << std::endl;
    std::cout << "  ident:  " << (unsigned long)ke.ident << std::endl;
    std::cout << "  filter: " << ke.filter << std::endl;
    std::cout << "  flags:  " << ke.flags << std::endl;
    std::cout << "  fflags: " << ke.fflags << std::endl;
    std::cout << "  data:   " << (long)ke.data << std::endl;
    std::cout << "  udata:  " << ke.udata << std::endl;
}

std::string Util::itoa(size_t value)
{
    std::ostringstream o;
    o << value;
    return std::string(o.str());
}

void Util::initHttpErrorMap(void)
{
    // 1xx Informational responses
    _httpErrorMap[100] = "Continue";
    _httpErrorMap[101] = "Switching Protocols";
    _httpErrorMap[102] = "Processing";

    // 2xx Success
    _httpErrorMap[200] = "OK";
    _httpErrorMap[201] = "Created";
    _httpErrorMap[202] = "Accepted";
    _httpErrorMap[203] = "Non-Authoritative Information";
    _httpErrorMap[204] = "No Content";
    _httpErrorMap[205] = "Reset Content";
    _httpErrorMap[206] = "Partial Content";
    _httpErrorMap[207] = "Multi-Status";
    _httpErrorMap[208] = "Already Reported";
    _httpErrorMap[226] = "IM Used";

    // 3xx Redirection
    _httpErrorMap[300] = "Multiple Choices";
    _httpErrorMap[301] = "Moved Permanently";
    _httpErrorMap[302] = "Found";
    _httpErrorMap[303] = "See Other";
    _httpErrorMap[304] = "Not Modified";
    _httpErrorMap[305] = "Use Proxy";
    _httpErrorMap[307] = "Temporary Redirect";
    _httpErrorMap[308] = "Permanent Redirect";

    // 4xx Client errors
    _httpErrorMap[400] = "Bad Request";
    _httpErrorMap[401] = "Unauthorized";
    _httpErrorMap[402] = "Payment Required";
    _httpErrorMap[403] = "Forbidden";
    _httpErrorMap[404] = "Not Found";
    _httpErrorMap[405] = "Method Not Allowed";
    _httpErrorMap[406] = "Not Acceptable";
    _httpErrorMap[407] = "Proxy Authentication Required";
    _httpErrorMap[408] = "Request Timeout";
    _httpErrorMap[409] = "Conflict";
    _httpErrorMap[410] = "Gone";
    _httpErrorMap[411] = "Length Required";
    _httpErrorMap[412] = "Precondition Failed";
    _httpErrorMap[413] = "Payload Too Large";
    _httpErrorMap[414] = "URI Too Long";
    _httpErrorMap[415] = "Unsupported Media Type";
    _httpErrorMap[416] = "Range Not Satisfiable";
    _httpErrorMap[417] = "Expectation Failed";
    _httpErrorMap[421] = "Misdirected Request";
    _httpErrorMap[422] = "Unprocessable Entity";
    _httpErrorMap[423] = "Locked";
    _httpErrorMap[424] = "Failed Dependency";
    _httpErrorMap[425] = "Too Early";
    _httpErrorMap[426] = "Upgrade Required";
    _httpErrorMap[428] = "Precondition Required";
    _httpErrorMap[429] = "Too Many Requests";
    _httpErrorMap[431] = "Request Header Fields Too Large";
    _httpErrorMap[451] = "Unavailable For Legal Reasons";

    // 5xx Server errors
    _httpErrorMap[500] = "Internal Server Error";
    _httpErrorMap[501] = "Not Implemented";
    _httpErrorMap[502] = "Bad Gateway";
    _httpErrorMap[503] = "Service Unavailable";
    _httpErrorMap[504] = "Gateway Timeout";
    _httpErrorMap[505] = "HTTP Version Not Supported";
    _httpErrorMap[506] = "Variant Also Negotiates";
    _httpErrorMap[507] = "Insufficient Storage";
    _httpErrorMap[508] = "Loop Detected";
    _httpErrorMap[510] = "Not Extended";
    _httpErrorMap[511] = "Network Authentication Required";
}

std::string Util::getErrorMessage(int statusCode)
{
    std::map<int, std::string>::const_iterator it = _httpErrorMap.find(statusCode);
    if (it != _httpErrorMap.end())
    {
        return it->second;
    }
    return "Unknown Error";
}