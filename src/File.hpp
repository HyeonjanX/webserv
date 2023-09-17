#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept> 
#include <vector>

class File
{
private:
    std::string _filepath;

public:
    File(void);

public:
    static bool         fileExists(const std::string &_filepath);
    static bool         checkWritePermission(const std::string &filepath);
    static std::string  getFile(const std::string &filepath);
    static std::string  readFile(const std::string &filepath);
    static bool         uploadFile(const std::string _filepath, const std::string &_content);
    static bool         writeFile(const std::string &_filepath, const std::string &_content);
};

#endif // FILE_HPP
