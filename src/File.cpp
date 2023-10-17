#include "File.hpp"
#include "Util.hpp"
#include <iostream>

#define DEBUG_PRINT false

File::File(void)
{
}

bool File::fileExists(const std::string &filepath, struct stat &fileInfo) { return (stat(filepath.c_str(), &fileInfo) == 0); }

bool File::isDirectory(const struct stat &fileInfo) { return S_ISDIR(fileInfo.st_mode); }

// bool File::checkFilePermission(const struct stat &fileInfo, mode_t mode) { return (fileInfo.st_mode & mode); }
bool File::checkFilePermission(const std::string &filepath, int mode) { return (access(filepath.c_str(), mode) == 0); }
bool File::checkFileExist(const std::string &filepath) { return checkFilePermission(filepath, F_OK); }
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

    if (DEBUG_PRINT) std::cout << "file " << filepath << " size: " << size << std::endl;

    file.seekg(0, std::ios::beg);
    if (file.fail())
    {
        file.close();
        std::cerr << "Failed to seekg()" << std::endl;
        throw std::runtime_error("Failed to seek to the beginning of the file");
    }

    std::string content;
    content.resize(size);
    if (!file.read(&content[0], size))
    {
        file.close();
        std::cerr << "Fail to read()" << std::endl;
        throw std::runtime_error("Fail to read");
    }

    if (DEBUG_PRINT) std::cout << "파일 읽기 성공: " << size << std::endl;

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

std::string File::getOnlyFile(const std::string &filepath)
{
    struct stat fileInfo;
    std::string content;

    if (!fileExists(filepath, fileInfo))
    {
        throw 404;
    }
    if (!checkFileReadPermission(filepath) || isDirectory(fileInfo))
    {
        throw 403;
    }
    try
    {
        content = readFile(filepath);
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
 * @param path 
 * @param filepath 
 * @param autoindex 
 * @param index 
 * @return std::string 디렉토링 리스팅용 HTML || 읽은 파일 데이터
 *
 * @throws int statusCode (404: 파일 존재, 403: 읽기 권한, 500: 읽기과정에서의 오류)
 */
std::string File::getFile(const std::string &path, const std::string &filepath, bool autoindex, const std::vector<std::string> &index, bool isHead)
{
    struct stat fileInfo;
    std::string content;
    
    if (DEBUG_PRINT) std::cout << "getFile: " << filepath << std::endl;
    if (!fileExists(filepath, fileInfo))
    {
        std::cerr << "존재하지 않음: " << filepath << std::endl;
        throw 404;
    }
    
    try
    {
        if (!isDirectory(fileInfo))
        {
            if (DEBUG_PRINT) std::cout << "파일임" << std::endl;
            if (!checkFileReadPermission(filepath))
            {
                std::cerr << "파일인데 권한 없음: " << filepath << std::endl;
                throw 403;
            }
            content = isHead ? std::string("") : readFile(filepath);
        }
        else if (/*isDiectory: true && */autoindex) 
        {
            if (DEBUG_PRINT) std::cout << "디렉토리 리스팅" << std::endl;
            if (!checkFileReadPermission(filepath))
            {
                std::cerr << "해당 폴더에 대한 권한 없음: " << filepath << std::endl;
                throw 403;
            }
            content = generateAutoIndexHTML(path, filepath);
        }
        else /* isDiectory: true && autoindex: false */
        {
            if (DEBUG_PRINT) std::cout << "디렉토리 index.size(): " << index.size() << std::endl;
            for (size_t i = 0; i < index.size(); ++i)
            {
                std::string newFilepath = filepath + std::string("/") + index[i];
                struct stat newFileInfo;
                if (DEBUG_PRINT) std::cout << "newFilepath: " << newFilepath << std::endl;
                if (!fileExists(newFilepath, newFileInfo))
                {
                    if (DEBUG_PRINT) std::cout << "미존재: " << newFilepath << std::endl;
                    continue;
                }
                if (isDirectory(newFileInfo) || !checkFileReadPermission(newFilepath))
                {
                    std::cerr << "디렉토리 or 권한 오류: " << newFilepath << std::endl;
                    throw 403;
                }
                if (DEBUG_PRINT) std::cout << "File: " << newFilepath << " 읽기 시도" << std::endl;
                
                content = isHead ? std::string("") : readFile(newFilepath);

                return content;
            }
            throw 404;
        }
    }
    catch(int statusCode)
    {
        throw statusCode;
    }
    catch(const std::runtime_error &e)
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
int File::canUploadFile(const std::string &root, const std::string &basename)
{
    const bool IS_DUPLICATE_OK = true; // for 테스터기, 하나의 정책 사항

    if (basename.empty()) // path == "/" 인 특수한 상황
        return 403;

    const std::string &filepath = root + basename;
    struct stat fileInfo, dirInfo;

	if (DEBUG_PRINT)
	{
	    std::cout << YELLOW << "======= File::uploadFile =======" << RESET << std::endl;
    	std::cout << YELLOW << "filepath: " << filepath << RESET << std::endl;
	}

    if (!IS_DUPLICATE_OK && fileExists(filepath, fileInfo))
    {
        return 409;
    }

    const std::string &dirPath = Util::extractDirPath(filepath);
	if (DEBUG_PRINT)
	{
		std::cout << YELLOW << "dirPath: " << dirPath << RESET << std::endl;
		std::cout << YELLOW << "!fileExists(dirPath, dirInfo): " << !fileExists(dirPath, dirInfo) << RESET << std::endl;
		std::cout << YELLOW << "!isDirectory: " << !isDirectory(dirInfo) << RESET << std::endl;
		std::cout << YELLOW << "!checkFileWritePermission: " << !checkFileWritePermission(dirPath) << RESET << std::endl;
	}

    if (!fileExists(dirPath, dirInfo) ||
        !isDirectory(dirInfo) ||
        !checkFileWritePermission(dirPath))
    {
        std::cerr << RED << "File canUploadFile: " << filepath << " >>> 403 에러" << RESET << std::endl;
        return 403;
    }

    return 0;
}

/**
 * @brief
 *
 * @param filepath
 * @param content
 * @return statusCode (200, 204: No content) 성공에도 여러 경우가 있어서 이렇게
 *
 * @throws int statusCode (409: 중복파일, 403: 디렉토리 유효성 && 쓰기 권한, 500: 쓰기 과정에서 실패)
 */
int File::uploadFile(const std::string &root, const std::string &basename, const std::string &content)
{
    const std::string &filepath = root + basename;

    int statusCode = canUploadFile(root, basename);

    if (statusCode)
    {
        throw statusCode;
    }

    if (!content.empty() && !writeFile(filepath, content))
    {
        throw 500;
    }

    statusCode = 201; // 201 Created

    std::cout << BLUE << "file upload " << statusCode << ", length: " << content.length() << RESET << std::endl;

    return statusCode;
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
std::string File::generateAutoIndexHTML(const std::string &path, const std::string &dirPath)
{
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
        // std::cout << "name: " << name << ", pass: " << Util::startsWith(name, ".") << std::endl;
        if (Util::startsWith(name, "."))
            continue;
        if (entry->d_type == DT_DIR) // 디렉터리일 경우 뒤에 /를 붙여줍니다.
            name += "/";
        const std::string &href = path.back() == '/' ?  path + name : path + std::string("/") + name; 
        htmlStream << "<a href=\"" << href << "\">" << name << "</a>" << "\n";
    }

    closedir(dir);

    htmlStream << "</pre>\n<hr>\n</body>\n</html>";

    return htmlStream.str();
}

int File::canExecuteFile(const std::string &filepath)
{
    struct stat fileInfo;
    std::string content;

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
