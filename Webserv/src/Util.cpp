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


bool Util::startsWith(const std::string &str, const std::string &prefix)
{
    if (str.length() < prefix.length())
    {
        return false;
    }

    for (std::size_t i = 0; i < prefix.length(); ++i)
    {
        if (str[i] != prefix[i])
        {
            return false;
        }
    }

    return true;
}

bool Util::endsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length())
    {
        return false;
    }

    std::string::const_reverse_iterator strIt = str.rbegin();
    std::string::const_reverse_iterator suffixIt = suffix.rbegin();

    for (; suffixIt != suffix.rend(); ++suffixIt, ++strIt)
    {
        if (*strIt != *suffixIt)
        {
            return false;
        }
    }

    return true;
}

/*
std::tolower와 ::tolower 두 함수 모두 존재하지만, 차이점이 있습니다.
std::tolower: 이 함수는 C++ 표준 라이브러리에서 제공되며, 일반적으로 오버로딩이 가능합니다. 즉, 다양한 타입에 대해 작동할 수 있습니다. std::tolower는 템플릿을 사용할 수 있기 때문에, 다양한 문자 타입을 지원할 수 있습니다. 하지만 이 함수는 locale을 매개변수로 받을 수 있는 버전도 있어서, std::transform과 같이 알고리즘 함수에 전달할 때 문제를 일으킬 수 있습니다.
::tolower: 이 함수는 전역 네임스페이스에 존재하는 C 표준 라이브러리의 함수입니다. 이 함수는 단순히 int 타입을 매개변수로 받아 int 타입을 반환합니다.
std::transform에 함수를 전달할 때 std::tolower를 사용하면, 어떤 오버로딩을 사용해야 할지 혼동이 발생할 수 있습니다. 그래서 이 경우에는 C 라이브러리의 ::tolower를 사용하여 오버로딩에 대한 혼동을 피하곤 합니다.
*/
std::string Util::toLowerCase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool Util::caseInsensitiveCompare(const std::string &str1, const std::string &str2)
{
    std::string lowerStr1 = toLowerCase(str1);
    std::string lowerStr2 = toLowerCase(str2);
    return lowerStr1 == lowerStr2;
}

std::vector<std::string>	Util::splitString
(std::string input, char delimiter)
{
	std::vector<std::string>	result;
	std::stringstream			ss(input);
	std::string					item;

	while (std::getline(ss, item, delimiter))
		result.push_back(item);
	return result;
}

std::string Util::ldtrim(const std::string &str, const std::string& delim)
{
	std::string	delimiter = " \t" + delim;

    std::string::size_type pos = str.find_first_not_of(delimiter);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(pos);
}

std::string Util::rdtrim(const std::string &str, const std::string& delim)
{
	std::string	delimiter = " \t" + delim;

    std::string::size_type pos = str.find_last_not_of(delimiter);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(0, pos + 1);
}

std::string Util::lrdtrim(const std::string &str, const std::string& delim)
{
    return ldtrim(rdtrim(str, delim), delim);
}

std::string	Util::removeDuplicate(std::string const& input)
{
	std::string	result;
	char		currentChar;
	bool		chars[256] = { false, };

	for (std::size_t i = 0; i < input.length(); ++i)
	{
		currentChar = input[i];
		if (!chars[static_cast<unsigned char>(currentChar)])
		{
			result += currentChar;
			chars[static_cast<unsigned char>(currentChar)] = true;
		}
	}
	return result;
}


// path는 /로 시작 보장 (HTTP/1.1 규칙)
std::string Util::extractBasename(const std::string &path)
{
    size_t lastSlash = path.find_last_of("/");

    if (lastSlash != std::string::npos)
    {
        return path.substr(lastSlash);
    }
    return std::string("");
}

std::string Util::extractDirPath(const std::string &path)
{
    size_t lastSlash = path.find_last_of("/");

    if (lastSlash != std::string::npos)
    {
        return path.substr(0, lastSlash);
    }
    return std::string("");
}

bool Util::hexToDecimal(const std::string &hexStr, size_t &decimalValue)
{
    char *end;
    errno = 0; // errno를 0으로 설정하여 이전의 오류를 지웁니다.
    size_t tempValue = std::strtoul(hexStr.c_str(), &end, 16);

	std::cout << "hexStr: |" << hexStr << "|" << std::endl;
	std::cout << "tempValue: |" << tempValue << "|" << std::endl;
	std::cout << "end: |" << *end << "|" << std::endl;

    // 변환에 실패하거나 값이 int의 범위를 벗어나면 false를 반환합니다.
    if (errno == ERANGE || *end != '\0')
    {
        return false;
    }

    decimalValue = tempValue;
    return true;
}

bool Util::hexToDecimalPositive(const std::string &hexStr, size_t &decimalValue)
{
    size_t tempValue;
    if (!Util::hexToDecimal(hexStr, tempValue) || tempValue < 0)
    {
        return false;
    }
    decimalValue = tempValue;
    return true;
}

bool Util::isLastChunk(const std::string &data)
{
    const std::string CRLF_STR("\r\n");
    size_t pos = data.find_first_not_of('0');

    // "CRLF" ...
    if (pos == 0)
    {
        return false;
    }
	
	// 1*("0")
    if (pos == std::string::npos)
    {
        return false;
    }
    // 1*("0") "CRLF"
	if (data.substr(pos, 2).compare(CRLF_STR) == 0)
    {
		// 헤더무시한다면
		if (data.substr(pos + 2, 2).compare(CRLF_STR) == 0)
		{
			std::cout << "ok" << std::endl;
			return true;
		}
    }
    return false;
}

std::size_t Util::tryReadChunk(const std::string &rawData, std::size_t &octetPos)
{
    const std::string CRLF_STR("\r\n");
    size_t size;
    size_t crlfPos1 = rawData.find(CRLF_STR);

    if (crlfPos1 == 0)
    {
        throw std::invalid_argument("None hex digit!");
    }

    if (crlfPos1 == std::string::npos)
    {
        return 0; // 아직 CRLF가 없어서 파싱 중단
    }

    std::string hexStr = rawData.substr(0, crlfPos1);

    if (!Util::hexToDecimalPositive(hexStr, size))
    {
        throw std::invalid_argument("Invalid hex digit");
    }

    if (rawData.size() < crlfPos1 + 2 + size + 2)
    {
        return 0;
    }

    size_t crlfPos2 = crlfPos1 + 2 + size;
	
    if (rawData.substr(crlfPos2, 2).compare(CRLF_STR) != 0)
    {
        throw std::invalid_argument("Invalid OCTET Line");
    }
	
	octetPos = crlfPos1 + 2;

    return size;
}

std::string Util::urlDecode(std::string s)
{
    std::ostringstream os;
    for (std::string::size_type i = 0; i < s.length(); ++i)
    {
        if (s[i] == '+')
        {
            os << ' ';
        }
        else if (s[i] == '%' && i + 2 < s.length())
        {
            int value;
            std::istringstream is(s.substr(i + 1, 2));
            if (is >> std::hex >> value)
            {
                os << static_cast<char>(value);
                i += 2;
            }
            else
            {
                // 잘못된 형식의 % 인코딩, 그냥 무시하거나 에러 처리
                os << '%';
            }
        } else {
            os << s[i];
        }
    }
    return os.str();
}
