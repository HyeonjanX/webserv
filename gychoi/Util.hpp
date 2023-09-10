#ifndef UTIL_HPP
#define UTIL_HPP

// Color
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define RESET "\033[0m"

#include <string>
#include <ctime>
#include <iostream>
#include <sstream>
#include <sys/event.h>
#include <map>

class Util
{
public:
    // static std::map<int, std::string> _httpErrorMap;
    // static void initHttpErrorMap(void);
    // static std::string getErrorMessage(int statusCode);

public:
    // trim
    static std::string ltrim(const std::string &str);
    static std::string rtrim(const std::string &str);
    static std::string lrtrim(const std::string &str);
    static std::string getDateString(void);

    static void print_kevent_info(const struct kevent &ke);
    static std::string ft_itoa(size_t value);
    static ssize_t ft_atol(const char *str, int base);

public:
};

#endif
