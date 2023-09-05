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
