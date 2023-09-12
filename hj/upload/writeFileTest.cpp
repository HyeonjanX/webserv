#include <map>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

bool fileExists(const std::string _filepath)
{
    struct stat info;
    return (stat(_filepath.c_str(), &info) == 0);
}

/**
 * 폴더의 쓰기 권한이 있는지 체크해야 한다.
 */
bool checkWritePermission(const std::string &filepath)
{
    std::string directory;
    size_t lastSlash = filepath.find_last_of("/");

    if (lastSlash == std::string::npos)
        directory = ".";
    else
        directory = filepath.substr(0, lastSlash);

    return (access(directory.c_str(), W_OK) != -1);
}

bool writeTextFile(const std::string &_filepath, const std::string &_content)
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

bool writeBinaryFile(const std::string &_filepath, const std::vector<char> &_content)
{
    std::ofstream outfile(_filepath.c_str(), std::ios::out | std::ios::binary);

    if (!outfile.is_open())
    {
        return false;
    }

    outfile.write(_content.data(), _content.size());

    if (outfile.fail())
    {
        outfile.close();
        return false;
    }

    outfile.close();

    return true;
}

bool writeUploadTextFile(const std::string &_filepath, const std::string &_content)
{
    if (fileExists(_filepath))
    {
        perror("409에러");
        throw 409;
    }
    if (!checkWritePermission(_filepath))
    {
        perror("403에러");
        throw 403;
    }
    if (!writeTextFile(_filepath, _content))
    {
        perror("500에러");
        throw 500;
    }
    return true;
}

bool writeUploadBinaryFile(const std::string &_filepath, std::vector<char> &_content)
{
    if (fileExists(_filepath))
    {
        perror("409에러");
        throw 409;
    }
    if (!checkWritePermission(_filepath))
    {
        perror("403에러");
        throw 403;
    }
    if (!writeBinaryFile(_filepath, _content))
    {
        perror("500에러");
        throw 500;
    }
    return true;
}

std::string makeContent(const char *s)
{
    std::ifstream infile(s);

    if (!infile.is_open())
    {
        std::cerr << "Failed to open file: " << s << std::endl;
        return std::string("");
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();

    return buffer.str();
}


/**
 * std::ios::ate: 파일을 열자마자 파일 포인터를 파일의 끝으로 이동시킵니다.
 * 이렇게 하면 tellg() 함수를 통해 쉽게 파일의 크기를 알 수 있습니다.
 * 그 후에 파일 포인터를 원하는 위치로 이동시켜 읽기 작업을 수행할 수 있습니다.
*/
std::vector<char> makeBinaryContent(const char *s)
{
    std::ifstream infile(s, std::ios::in | std::ios::binary | std::ios::ate);

    if (!infile.is_open())
    {
        std::cerr << "Failed to open file: " << s << std::endl;
        return std::vector<char>();
    }

    std::streamsize size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!infile.read(buffer.data(), size))
    {
        std::cerr << "Failed to read file: " << s << std::endl;
        return std::vector<char>();
    }

    return buffer;
}

void test1(const char *testFile, std::string _filepath)
{
    std::string _content = makeContent(testFile);

    try
    {
        writeUploadTextFile(_filepath, _content);
    }
    catch (int statusCode)
    {
        std::cerr << "오류발생: " << statusCode << std::endl;
    }
}

void test2(const char *testFile, std::string _filepath)
{
    std::vector<char> _content = makeBinaryContent(testFile);

    try
    {
        writeUploadBinaryFile(_filepath, _content);
    }
    catch (int statusCode)
    {
        std::cerr << "오류발생: " << statusCode << std::endl;
    }
}

int main()
{
    test1("./test.txt", std::string("./outfile.txt")); // txt test ok
    test2("./test.png", std::string("./outfile.png")); // png test ok 
    test2("./idle.mp4", std::string("./outfule.mp4")); // mp4 test ok
}