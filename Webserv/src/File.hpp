#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept> 
#include <vector>

// autoindex
#include <sstream>
#include <dirent.h>
#include <sys/types.h>

class File
{
private:
    std::string _filepath;

public:
    File(void);

public:
    static bool         fileExists(const std::string &_filepath, struct stat &fileInfo);

    static bool         isDirectory(const struct stat &fileInfo);

    static bool         checkFilePermission(const std::string &filepath, int mode);
    static bool         checkFileReadPermission(const std::string &filepath);
    static bool         checkFileWritePermission(const std::string &filepath);
    static bool         checkFileExecutePermission(const std::string &filepath);

    static bool         checkWritePermission(const std::string &filepath);
    static std::string  getFile(const std::string &root, const std::string &path, bool autoindex = false); // GET
    static std::string  readFile(const std::string &filepath);
    static bool         uploadFile(const std::string _filepath, const std::string &_content); // POST
    static bool         writeFile(const std::string &_filepath, const std::string &_content);
    static bool         deleteFile(const std::string &filepath); // DELETE

    static int          canUploadFile(const std::string filepath);
    static int          canExecuteFile(const std::string& filepath);

    static std::string  generateAutoIndexHTML(const std::string &root, const std::string &path);
};

#endif // FILE_HPP
