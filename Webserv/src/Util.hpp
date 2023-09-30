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
#include <cctype> // for isalnum
#include <algorithm>
#include <vector>

#define B_CHAR_NO_SPACE "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'()+_,-./:=?"; // for boundary

struct ReplaceInvalidChar
{
    bool operator()(char c)
    {
        return !std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-';
    }
};

class Util
{

public:
    static std::map<int, std::string> _httpErrorMap;
    static void initHttpErrorMap(void);
    static std::string getStatusCodeMessage(int statusCode);

public:
    // trim
    static std::string ltrim(const std::string &str);
    static std::string rtrim(const std::string &str);
    static std::string lrtrim(const std::string &str);
    static std::string getDateString(void);

    static void print_kevent_info(const struct kevent &ke);
    static std::string ft_itoa(size_t value);
    static ssize_t ft_atol(const char *str, int base);

    static std::string sanitizeFilename(const std::string &filename);
    static bool startsWith(const std::string &str, const std::string &prefix);
    static bool endsWith(const std::string &str, const std::string &suffix);
    static std::string toLowerCase(const std::string &input);
    static bool caseInsensitiveCompare(const std::string &str1, const std::string &str2);

    static std::vector<std::string>
	splitString(std::string input, char delimiter);
    static std::string ldtrim(const std::string &str, const std::string& delim);
    static std::string rdtrim(const std::string &str, const std::string& delim);
    static std::string lrdtrim(const std::string &str, const std::string& delim);
	static std::string removeDuplicate(std::string const& input);
    static std::string extractBasename(const std::string &path);
    static std::string extractDirPath(const std::string &path);

    static bool         hexToDecimal(const std::string &hexStr, size_t &decimalValue);
    static bool         hexToDecimalPositive(const std::string &hexStr, size_t &decimalValue);
    static bool         isLastChunk(const std::string &data);
    static std::size_t  tryReadChunk(const std::string &rawData, std::size_t &octetPos);

    static std::string  urlDecode(std::string s);
    static bool         isValidBoundary(const std::string& boundary);
};

#endif