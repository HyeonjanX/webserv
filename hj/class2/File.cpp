#include "File.hpp"
#include <iostream>

File::File(void)
{

}

bool File::fileExists(const std::string &_filepath)
{
    struct stat info;
    return (stat(_filepath.c_str(), &info) == 0);
}

bool File::checkWritePermission(const std::string &filepath)
{
    std::string directory;
    size_t lastSlash = filepath.find_last_of("/");

    if (lastSlash == std::string::npos)
        directory = ".";
    else
        directory = filepath.substr(0, lastSlash);

    return (access(directory.c_str(), W_OK) != -1);
}

bool File::writeTextFile(const std::string &_filepath, const std::string &_content)
{
    std::ofstream outfile(_filepath.c_str());

    if (!outfile.is_open())
    {
        return false;
    }

    outfile << _content;

    if (outfile.fail())
    {
        outfile.close();
        return false;
    }

    outfile.close();
    return true;
}

bool File::writeUploadTextFile(const std::string _filepath, const std::string &_content)
{
    if (fileExists(_filepath))
    {
        perror("writeUploadTextFile: 409 conflict");
        throw 409;
    }
    if (!checkWritePermission(_filepath))
    {
        perror("writeUploadTextFile: 403 Forbidden");
        throw 403;
    }
    if (!writeTextFile(_filepath, _content))
    {
        perror("writeUploadTextFile: 500");
        throw 500;
    }
    return true;
}

std::string File::readFile(const std::string &filePath)
{
    char buffer[1024];

    std::string content;

    struct stat fileStat;

    if (stat(filePath.c_str(), &fileStat) != 0)
    {
        throw 404;
    }

    else if (!(fileStat.st_mode & S_IRUSR))
    {
        throw 403;
    }

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file)
    {
        throw 500;
    }

    while (!file.eof())
    {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0)
        {
            content.append(buffer, bytesRead);
        }
        else
        {
            file.close();
            throw 500;
        }
    }

    file.close();

    return content;
}