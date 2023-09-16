#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

class File
{
private:
    std::string _filepath;

public:
    File(void);

public:
    static bool         fileExists(const std::string &_filepath);
    static bool         checkWritePermission(const std::string &filepath);
    static bool         writeTextFile(const std::string &_filepath, const std::string &_content);
    static bool         writeUploadTextFile(const std::string _filepath, const std::string &_content);
    static std::string  readFile(const std::string &filePath);
};

#endif // FILE_HPP
