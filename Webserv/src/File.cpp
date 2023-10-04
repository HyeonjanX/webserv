#include "File.hpp"
#include "Util.hpp"
#include <iostream>

File::File(void)
{
}

bool File::fileExists(const std::string &filepath, struct stat &fileInfo) { return (stat(filepath.c_str(), &fileInfo) == 0); }

bool File::isDirectory(const struct stat &fileInfo) { return S_ISDIR(fileInfo.st_mode); }

// bool File::checkFilePermission(const struct stat &fileInfo, mode_t mode) { return (fileInfo.st_mode & mode); }
bool File::checkFilePermission(const std::string &filepath, int mode) { return (access(filepath.c_str(), mode) == 0); }
bool File::checkFileReadPermission(const std::string &filepath) { return checkFilePermission(filepath, R_OK); }
bool File::checkFileWritePermission(const std::string &filepath) { return checkFilePermission(filepath, W_OK); }
bool File::checkFileExecutePermission(const std::string &filepath) { return checkFilePermission(filepath, X_OK); }


// 디렉터리인지 확인 (stat 함수를 사용해야 함)
// 이 함수는 access로 대체할 수 없으므로 그대로 둡니다.


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

/**
 * @brief
 *
 * @param filepath
 * @return std::string 읽은 파일 데이터
 *
 * @throws std::runtime_error 정상적인 읽기 시도 과정에서 실패시 어떤 과정에서 실패했는지
 */
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

/**
 * @brief
 *
 * @param filepath
 * @param content
 * @return true 쓰기 성공
 * @return false 쓰기 실패
 */
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

/**
 * @brief
 *
 * @param root
 * @param path
 * @param autoindex
 * @return std::string 디렉토링 리스팅용 HTML || 읽은 파일 데이터
 *
 * @throws int statusCode (404: 파일 존재, 403: 읽기 권한, 500: 읽기과정에서의 오류)
 */
std::string File::getFile(const std::string &root, const std::string &path, bool autoindex)
{
    struct stat fileInfo;
    std::string content;
    std::string filepath = root + path;

    if (!fileExists(filepath, fileInfo))
    {
        throw 404;
    }
    if (!checkFileReadPermission(filepath) ||
        (isDirectory(fileInfo) && !autoindex))
    {
        throw 403;
    }
    try
    {
        content = isDirectory(fileInfo) ? generateAutoIndexHTML(root, path) : readFile(filepath);
    }
    catch (std::runtime_error &e)
    {
        // readFile과정에서 실패시, 어떤 사유로 에러가 발생했는지 확인할 수 있다.
        std::cerr << e.what() << std::endl;
        throw 500;
    }

    return content;
}

/**
 * @brief
 *
 * @param filepath
 * @return int statusCode (409: 중복파일, 403: 디렉토리 유효성 && 쓰기 권한)
 *
 */
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
    std::cout << YELLOW << "checkFileWritePermission: " << !checkFileWritePermission(dirPath) << RESET << std::endl;

    if (!fileExists(dirPath, dirInfo) ||
        !isDirectory(dirInfo) ||
        !checkFileWritePermission(dirPath))
    {
        return 403;
    }

    return 0;
}

/**
 * @brief
 *
 * @param filepath
 * @param content
 * @return true 업로드 성공
 * @return false 업로드 실패
 *
 * @throws int statusCode (409: 중복파일, 403: 디렉토리 유효성 && 쓰기 권한, 500: 쓰기 과정에서 실패)
 */
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

/**
 * @brief
 *
 * @param filepath
 * @return true 삭제 성공
 * @return false
 *
 * @throws int statusCode (404: 파일 미존재, 403: 쓰기 권한 X, 500: ::remove()호출 실패)
 */
bool File::deleteFile(const std::string &filepath)
{
    struct stat fileInfo;

    if (!fileExists(filepath, fileInfo))
    {
        throw 404;
    }
    if (isDirectory(fileInfo) || !checkFileWritePermission(filepath))
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

/**
 * @brief
 *
 * @param root
 * @param path
 * @return std::string 리스팅용 HTML
 *
 * @throws int statusCode (500: 읽기 도중 알 수 없는 실패.)
 */
std::string File::generateAutoIndexHTML(const std::string &root, const std::string &path)
{
    std::string dirPath = root + path;
    std::ostringstream htmlStream;

    DIR *dir = opendir(dirPath.c_str());
    if (dir == NULL)
    {
        throw 500; // "Error opening directory.";
    }

    htmlStream << "<html>\n<head>\n<title>Index of " << path << "</title>\n</head>\n";
    htmlStream << "<body>\n<h1>Index of " << path << "</h1>\n<hr>\n<pre>\n";

    if (path.compare("/"))
    {
        htmlStream << "<a href=\"../\">../</a>\n";
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        std::cout << "name: " << name << ", pass: " << Util::startsWith(name, ".") << std::endl;
        if (Util::startsWith(name, "."))
        {
            continue;
        }
        if (entry->d_type == DT_DIR) // 디렉터리일 경우 뒤에 /를 붙여줍니다.
        {
            name += "/";
        }
        htmlStream << "<a href=\"" << name << "\">" << name << "</a>"
                   << "\n";
        // htmlStream << "<a href=\"" << name << "\">" << name << "</a>";
    }

    closedir(dir);

    htmlStream << "</pre>\n<hr>\n</body>\n</html>";

    return htmlStream.str();
}

int File::canExecuteFile(const std::string &filepath)
{
    struct stat fileInfo;
    std::string content;

    std::cout << "실행 체크: " << filepath << std::endl;

    if (!fileExists(filepath, fileInfo))
    {
        return 404; // File not found
    }
    if (!checkFileExecutePermission(filepath) || isDirectory(fileInfo))
    {
        return 403; // Forbidden
    }

    return 0;
}
