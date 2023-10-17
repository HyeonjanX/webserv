#include "Cgi.hpp"
#include "Util.hpp"

Cgi::Cgi() : _pid(0), _status(0), _sendBytes(0)
{
    _inPipe[READ_FD] = -1;
    _inPipe[WRITE_FD] = -1;
    _outPipe[READ_FD] = -1;
    _outPipe[WRITE_FD] = -1;
}

Cgi::Cgi(const Cgi &other) { *this = other; }

Cgi &Cgi::operator=(const Cgi &other)
{
    if (this != &other)
    {

        _inPipe[READ_FD] = other._inPipe[READ_FD];
        _inPipe[WRITE_FD] = other._inPipe[WRITE_FD];
        _outPipe[READ_FD] = other._outPipe[READ_FD];
        _outPipe[WRITE_FD] = other._outPipe[WRITE_FD];

        _pid = other._pid;
        _status = other._status;

        _env = other._env;

        _readData = other._readData;
        _postData = other._postData;
        _sendBytes = other._sendBytes;
    }
    return *this;
}

Cgi::~Cgi() {}

static char **convertToCArray(const std::vector<std::string> &vec)
{
    char **cArray = new char *[vec.size() + 1];

    for (size_t i = 0; i < vec.size(); ++i)
    {
        cArray[i] = new char[vec[i].length() + 1];
        std::strcpy(cArray[i], vec[i].c_str());
    }

    cArray[vec.size()] = 0;

    return cArray;
}

static void freeCArray(char **cArray, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        delete[] cArray[i];
    delete[] cArray;
}

int &Cgi::getInPipe(int idx) { return _inPipe[idx]; }
int &Cgi::getOutPipe(int idx) { return _outPipe[idx]; }
const std::string &Cgi::getReadData() { return _readData; };
bool Cgi::allSend() { return _sendBytes >= _postData.size(); }

const std::string &Cgi::getPostData() const { return _postData; }
void Cgi::setPostData(std::string postData) { _postData = postData; }

int Cgi::clearCgi()
{
    int exitCode = 0;

    clearPipe();
    exitCode = clearChild();
    _pid = 0;
    _status = 0;
    _env.clear();

    _readData.clear();
    _postData.clear();
    _sendBytes = 0;

    return exitCode;
}

int Cgi::clearChild()
{
    int exitCode = 0;

    if (_pid)
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, &exitCode, WNOHANG);
        _pid = 0;
    }

    return WEXITSTATUS(exitCode);
}

void Cgi::clearPipe()
{
    closePipe(_inPipe[READ_FD]);
    closePipe(_inPipe[WRITE_FD]);
    closePipe(_outPipe[READ_FD]);
    closePipe(_outPipe[WRITE_FD]);
}

void Cgi::closePipe(int &fd)
{
    if (fd != -1 && close(fd) == -1)
    {
        std::cerr << "closePipe error: " << strerror(errno) << std::endl;
    }
    fd = -1;
}

/**
 * @brief
 *
 * @param method
 *
 * @throws const char *msg 어떤 상황에서 오류가 난 것인지 포함
 */
void Cgi::exec(const std::string &method, const std::string &programPath, const std::vector<std::string> &argv)
{
    // 5. pipe 생성 && 논블록 처리
    if ((pipe(_outPipe) == -1 || fcntl(_outPipe[READ_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1) ||
        (pipe(_inPipe) == -1 || fcntl(_inPipe[WRITE_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1))
    {
        throw "exec과정에서 pipe() or fcntl() 실패";
    }

    if ((_pid = fork()) == -1)
    {
        throw "exec과정에서 fork() 실패";
    }
    else if (_pid == 0)
    {
        // 자녀 프로세스
        if (dup2(_outPipe[WRITE_FD], STDOUT_FILENO) == -1 || dup2(_inPipe[READ_FD], STDIN_FILENO) == -1)
            throw ExecveException();

        clearPipe();

        char **argvArray = convertToCArray(argv);
        char **envpArray = convertToCArray(_env);

        execve(programPath.c_str(), argvArray, envpArray);

        freeCArray(argvArray, argv.size());
        freeCArray(envpArray, _env.size());

        throw ExecveException();
    }

    // 6. 부모 => 안 쓰는 파이프 닫기
    closePipe(_inPipe[READ_FD]);
    closePipe(_outPipe[WRITE_FD]);
    if (method.compare("POST") != 0 || _postData.empty())
        closePipe(_inPipe[WRITE_FD]);
}

/**
 * @brief 
 * 
 * @throws int statusCode 500
 */
void Cgi::writePipe()
{
    ssize_t bytes = write(_inPipe[WRITE_FD], _postData.data() + _sendBytes, _postData.size() - _sendBytes);
    if (bytes == -1)
    {
        std::cerr << "Fail to write(): " << strerror(errno) << std::endl;
        throw 500;
    }
    _sendBytes += bytes;

    if (allSend())
        closePipe(_inPipe[WRITE_FD]);
}

/**
 * @brief 
 * 
 * @throws int statusCode 500
 */
size_t Cgi::readPipe()
{
    static std::vector<char> buffer(READ_BUFFER_SIZE);

    ssize_t readBytes = read(_outPipe[READ_FD], buffer.data(), READ_BUFFER_SIZE);

    if (readBytes == -1)
    {
        std::cerr << "Fail to read(). fd: " << _outPipe[READ_FD] << ", " << std::string(strerror(errno)) << std::endl;
        throw 500;
    }

    _readData.append(buffer.data(), readBytes);

    return readBytes;
}

// 탐색시 getInPipe[WRITE_FD], getOutPipe[READ_FD]만 필요하다.
bool Cgi::isPipe(int fd)
{
    return fd != -1 && (_inPipe[WRITE_FD] == fd || _outPipe[READ_FD] == fd);
}

void Cgi::pipePrint() const
{
    std::cout << RED << "pipe check: "
              << _inPipe[READ_FD] << ", "
              << _inPipe[WRITE_FD] << ", "
              << _outPipe[READ_FD] << ", "
              << _outPipe[WRITE_FD] << RESET << std::endl;
}

const char *Cgi::ExecveException::what() const throw()
{
    return "CGI 프로그램 execve() 실패";
}

void Cgi::setEnvFromRequestHeaders(Request &request, std::string method, std::string path)
{
    _env.clear();
    _env.push_back("REQUEST_METHOD=" + method);
    _env.push_back("SERVER_PROTOCOL=" + std::string("HTTP/1.1"));
    _env.push_back("PATH_INFO=" + path);
    _env.push_back("CONTENT_TYPE=" + request.findHeaderValue("content-type"));
    _env.push_back("CONTENT_LENGTH=" + std::to_string(request.getPostData().size()));
    _env.push_back("HTTP_X_SECRET_HEADER_FOR_TEST=" + request.findHeaderValue("x-secret-header-for-test"));
}

size_t Cgi::getSendBytes() const { return _sendBytes; };
