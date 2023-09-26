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
    static bool         fileExists(const std::string &_filepath, struct stat &fileInfo);

    static bool         isDirectory(const struct stat &fileInfo);

    static bool         checkFilePermission(const struct stat &fileInfo, mode_t mode);
    
    static bool         checkFileReadPermission(const struct stat &fileInfo);
    static bool         checkFileWritePermission(const struct stat &fileInfo);
    static bool         checkFileExcutePermission(const struct stat &fileInfo);

    static bool         checkWritePermission(const std::string &filepath);
    static std::string  getFile(const std::string &filepath); // GET
    static std::string  readFile(const std::string &filepath);
    static bool         uploadFile(const std::string _filepath, const std::string &_content); // POST
    static bool         writeFile(const std::string &_filepath, const std::string &_content);
    static bool         deleteFile(const std::string &filepath); // DELETE

    static int          canUploadFile(const std::string filepath);
};

#endif // FILE_HPP
