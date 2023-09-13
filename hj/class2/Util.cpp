#include "Util.hpp"

std::map<int, std::string> Util::_httpErrorMap;

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

std::string Util::ft_itoa(size_t value)
{
    std::ostringstream o;
    o << value;
    return o.str();
}

long Util::ft_atol(const char *str, int base)
{
    char *end;

    long num = std::strtol(str, &end, base);

    if (*end != 0)
    {
        throw "Parsing failed. Remaining string: " + std::string(end);
    }

    return num;
}

void Util::initHttpErrorMap(void)
{
    // 1xx Informational responses
    Util::_httpErrorMap[100] = "Continue";
    Util::_httpErrorMap[101] = "Switching Protocols";
    Util::_httpErrorMap[102] = "Processing";

    // 2xx Success
    Util::_httpErrorMap[200] = "OK";
    Util::_httpErrorMap[201] = "Created";
    Util::_httpErrorMap[202] = "Accepted";
    Util::_httpErrorMap[203] = "Non-Authoritative Information";
    Util::_httpErrorMap[204] = "No Content";
    Util::_httpErrorMap[205] = "Reset Content";
    Util::_httpErrorMap[206] = "Partial Content";
    Util::_httpErrorMap[207] = "Multi-Status";
    Util::_httpErrorMap[208] = "Already Reported";
    Util::_httpErrorMap[226] = "IM Used";

    // 3xx Redirection
    Util::_httpErrorMap[300] = "Multiple Choices";
    Util::_httpErrorMap[301] = "Moved Permanently";
    Util::_httpErrorMap[302] = "Found";
    Util::_httpErrorMap[303] = "See Other";
    Util::_httpErrorMap[304] = "Not Modified";
    Util::_httpErrorMap[305] = "Use Proxy";
    Util::_httpErrorMap[307] = "Temporary Redirect";
    Util::_httpErrorMap[308] = "Permanent Redirect";

    // 4xx Client errors
    Util::_httpErrorMap[400] = "Bad Request";
    Util::_httpErrorMap[401] = "Unauthorized";
    Util::_httpErrorMap[402] = "Payment Required";
    Util::_httpErrorMap[403] = "Forbidden";
    Util::_httpErrorMap[404] = "Not Found";
    Util::_httpErrorMap[405] = "Method Not Allowed";
    Util::_httpErrorMap[406] = "Not Acceptable";
    Util::_httpErrorMap[407] = "Proxy Authentication Required";
    Util::_httpErrorMap[408] = "Request Timeout";
    Util::_httpErrorMap[409] = "Conflict";
    Util::_httpErrorMap[410] = "Gone";
    Util::_httpErrorMap[411] = "Length Required";
    Util::_httpErrorMap[412] = "Precondition Failed";
    Util::_httpErrorMap[413] = "Payload Too Large";
    Util::_httpErrorMap[414] = "URI Too Long";
    Util::_httpErrorMap[415] = "Unsupported Media Type";
    Util::_httpErrorMap[416] = "Range Not Satisfiable";
    Util::_httpErrorMap[417] = "Expectation Failed";
    Util::_httpErrorMap[421] = "Misdirected Request";
    Util::_httpErrorMap[422] = "Unprocessable Entity";
    Util::_httpErrorMap[423] = "Locked";
    Util::_httpErrorMap[424] = "Failed Dependency";
    Util::_httpErrorMap[425] = "Too Early";
    Util::_httpErrorMap[426] = "Upgrade Required";
    Util::_httpErrorMap[428] = "Precondition Required";
    Util::_httpErrorMap[429] = "Too Many Requests";
    Util::_httpErrorMap[431] = "Request Header Fields Too Large";
    Util::_httpErrorMap[451] = "Unavailable For Legal Reasons";

    // 5xx Server errors
    Util::_httpErrorMap[500] = "Internal Server Error";
    Util::_httpErrorMap[501] = "Not Implemented";
    Util::_httpErrorMap[502] = "Bad Gateway";
    Util::_httpErrorMap[503] = "Service Unavailable";
    Util::_httpErrorMap[504] = "Gateway Timeout";
    Util::_httpErrorMap[505] = "HTTP Version Not Supported";
    Util::_httpErrorMap[506] = "Variant Also Negotiates";
    Util::_httpErrorMap[507] = "Insufficient Storage";
    Util::_httpErrorMap[508] = "Loop Detected";
    Util::_httpErrorMap[510] = "Not Extended";
    Util::_httpErrorMap[511] = "Network Authentication Required";
}

std::string Util::getStatusCodeMessage(int statusCode)
{
    static std::string defaultMessage = std::string("Unknown Error");
    if (Util::_httpErrorMap.empty())
    {
        Util::initHttpErrorMap();
    }
    std::map<int, std::string>::const_iterator it = Util::_httpErrorMap.find(statusCode);
    if (it != Util::_httpErrorMap.end())
    {
        return it->second;
    }
    return defaultMessage;
}


std::string Util::sanitizeFilename(const std::string &filename)
{
    // 경로 정보 제거
    std::string::size_type last_slash = filename.find_last_of("/\\");
    
    std::string sanitized = last_slash == std::string::npos ? filename : filename.substr(last_slash + 1);

    std::replace_if(sanitized.begin(), sanitized.end(), ReplaceInvalidChar(), '_');
    
    return sanitized;
}