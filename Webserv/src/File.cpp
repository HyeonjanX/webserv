#include "File.hpp"
#include <iostream>

File::File(void)
{
}

bool File::fileExists(const std::string &filepath)
{
    struct stat info;
    return (stat(filepath.c_str(), &info) == 0);
}

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

bool File::uploadFile(const std::string filepath, const std::string &content)
{
    if (fileExists(filepath))
    {
        perror("writeUploadTextFile: 409 conflict");
        throw 409;
    }
    if (!checkWritePermission(filepath))
    {
        perror("writeUploadTextFile: 403 Forbidden");
        throw 403;
    }
    if (!writeFile(filepath, content))
    {
        perror("writeUploadTextFile: 500");
        throw 500;
    }
    return true;
}

std::string File::readFile(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
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

std::string File::getFile(const std::string &filepath)
{
    std::string content;
    struct stat fileStat;

    if (stat(filepath.c_str(), &fileStat) != 0)
    {
        throw 404;
    }

    else if (!(fileStat.st_mode & S_IRUSR))
    {
        throw 403;
    }

    try
    {
        content = readFile(filepath);
    }
    catch (const char *errmsg)
    {
        std::cerr << errmsg << std::endl;
        throw 500;
    }

    return content;
}