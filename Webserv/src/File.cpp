#include "File.hpp"
#include "Util.hpp"
#include <iostream>

File::File(void)
{
}

bool File::fileExists(const std::string &filepath, struct stat &fileInfo) { return (stat(filepath.c_str(), &fileInfo) == 0); }

bool File::isDirectory(const struct stat &fileInfo) { return S_ISDIR(fileInfo.st_mode); }

bool File::checkFilePermission(const struct stat &fileInfo, mode_t mode) { return (fileInfo.st_mode & mode); }

bool File::checkFileReadPermission(const struct stat &fileInfo) { return checkFilePermission(fileInfo, S_IRUSR); }
bool File::checkFileWritePermission(const struct stat &fileInfo) { return checkFilePermission(fileInfo, S_IWUSR); }
bool File::checkFileExcutePermission(const struct stat &fileInfo) { return checkFilePermission(fileInfo, S_IXUSR); }

// 현재 안 쓰고 있는 함수, 나중에 확정적으로 필요없으면 지우면 됨
// POST의 경우, path가 들어와도 전체를 사용하지 않고, 
bool File::checkWritePermission(const std::string &filepath)
{
    std::string directory;
    size_t lastSlash = filepath.find_last_of("/");

    if (lastSlash == std::string::npos)
        directory = ".";
    else
        directory = filepath.substr(0, lastSlash); // /a/b/c/file => /a/b/c

    return (access(directory.c_str(), W_OK) != -1);
}

std::string File::readFile(const std::string &filepath)
{
    std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Fail to start ifstream");
    }

    std::streamsize size = file.tellg();
    if (size == -1)
    {
        file.close();
        throw std::runtime_error("Failed to get file size");
    }

    file.seekg(0, std::ios::beg);
    if (file.fail())
    {
        file.close();
        throw std::runtime_error("Failed to seek to the beginning of the file");
    }

    std::string content;
    content.resize(size);
    if (!file.read(&content[0], size))
    {
        file.close();
        throw std::runtime_error("Fail to read");
    }

    file.close();
    return content;
}

bool File::writeFile(const std::string &filepath, const std::string &content)
{
    // std::ofstream outfile(filepath.c_str());
    std::ofstream outfile(filepath.c_str(), std::ios::binary);

    if (!outfile.is_open())
    {
        return false;
    }

    outfile << content;

    if (outfile.fail())
    {
        outfile.close();
        return false;
    }

    outfile.close();
    return true;
}

std::string File::getFile(const std::string &filepath)
{
    struct stat fileInfo;
    std::string content;

    if (!fileExists(filepath, fileInfo))
    {
        throw 404;
    }
    if (!checkFileReadPermission(fileInfo))
    {
        throw 403;
    }
    try
    {
        content = readFile(filepath);
    }
    catch (std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        throw 500;
    }

    return content;
}

int File::canUploadFile(const std::string filepath)
{
    struct stat fileInfo, dirInfo;

    std::cout << YELLOW << "======= File::uploadFile =======" << RESET << std::endl;
    std::cout << YELLOW << "filepath: " << filepath << RESET << std::endl;
    
    if (fileExists(filepath, fileInfo))
    {
        return 409;
    }

    const std::string &dirPath = Util::extractDirPath(filepath);
    std::cout << YELLOW << "dirPath: " << dirPath << RESET << std::endl;
    std::cout << YELLOW << "fileExists(dirPath, dirInfo): " << fileExists(dirPath, dirInfo) << RESET << std::endl;
    std::cout << YELLOW << "isDirectory: " << isDirectory(dirInfo) << RESET << std::endl;
    std::cout << YELLOW << "checkFileWritePermission: " << !checkFileWritePermission(dirInfo) << RESET << std::endl;

    if (!fileExists(dirPath, dirInfo) ||
        !isDirectory(dirInfo) ||
        !checkFileWritePermission(dirInfo))
    {
        return 403;
    }

    return 0;
}

bool File::uploadFile(const std::string filepath, const std::string &content)
{
    int statusCode = canUploadFile(filepath);

    if (statusCode)
    {
        throw statusCode;
    }

    if (!writeFile(filepath, content))
    {
        throw 500;
    }

    std::cout << BLUE << "file upload ok: " << content.length() << RESET << std::endl;

    return true;
}

bool File::deleteFile(const std::string &filepath)
{
    struct stat fileInfo;

    if (!fileExists(filepath, fileInfo))
    {
        throw 404;
    }
    if (isDirectory(fileInfo) || !checkFileWritePermission(fileInfo))
    {
        throw 403;
    }
    // 실제 삭제여부 고민.
    if (::remove(filepath.c_str())) // <cstdio>의 파일 삭제 함수
    {
        throw 500;
    };

    return true;
}