
#include "Client.hpp"

Client::Client(int serverSocket)
    : _contentLength(0), _status(0), _ischunk(0)
{
    std::memset(&_addr, 0, sizeof(_addr));
    socklen_t addr_size = static_cast<socklen_t>(sizeof(_addr));
    if ((_socket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&_addr), &addr_size)) == -1)
    {
        throw "Fail to accept()" + std::string(strerror(errno));
    }
    // PDF에 FD_CLOEXEC 까지 쓰라는거 아닌가?
    if (fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC) == -1)
    {
        throw "fcntl() error" + std::string(strerror(errno));
    }
    std::cout << "Client 생성: " << _socket << std::endl;
}

Client::~Client(void) {}

int Client::readProcess(void)
{
    char buffer[1024];
    ssize_t bytes_read = read(_socket, buffer, 1024);

    if (bytes_read == 0)
    {
        // 클라이언트와의 정상 종료? => 재사용을 위해  => Request와 Response 초기화필요할듯
        std::cout << "클라이언트의 연결이 종료되었습니다." << std::endl;
    }

    else if (bytes_read == -1)
    {
        // errno 체크 X 조건 => 에러로 처리
        // 클라이언트 연결 종료 => 단, 종료전 500류의 에러를 보낼것인지 판단.
    }

    else
    {
        // 읽기 성공 => 처리

        _data.append(buffer, bytes_read);

        std::cout << "------------------버퍼 데이터------------------" << std::endl;
        std::cout << buffer << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        if (_status == READ_HEADER)
        {
            // 1. 헤더 끝까지 읽기
            size_t pos = _data.find("\r\n\r\n");
            if (pos == std::string::npos)
            {
                _header.append(buffer, bytes_read);
            }
            else
            {
                _status = READ_BODY;
                if (false)
                {
                    // 청크 수신 모드라면
                    _ischunk = true;
                }

                // 헤더를 읽었으니, requestLine의 유효성 판별이나
                // Content-Length들을 파악해 본문을 얼마나 읽으면 되는지 파악

                _header.append(buffer, pos + 2); // 헤더가 \r\n으로 구분가능 하도록!
                _body.append(buffer + pos + 4, bytes_read - pos - 4);

                std::cout << "클라이언트 입력에서 헤더를 찾았습니다." << std::endl;
            }

            if (_header.size() >= _headaerLimit)
            {
                // 헤더 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
            }
        }
        else
        {
            _body.append(buffer, bytes_read);
        }

        if (_status == READ_BODY)
        {
            // 2. 본문 끝까지 읽기

            // 2.1 본문 길이 초과 체크
            if (_body.size() >= _bodyLimit)
            {
                // 바디 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
            }

            if (_ischunk)
            {
                // 2.1 chuncked 처리
                size_t pos = _body.find("\r\n\r\n");
                if (pos != std::string::npos)
                {
                    // 끝을 찾음
                    if (pos + 4 == _body.size())
                    {
                        // \r\n\r\n으로 끝나야 유효한 요청.
                        // \r\n단위로 짤라서 뭔가 하기...
                        chunkRead();
                        _status = READ_END;
                    }
                    else
                    {
                        // 유효하지 않은 요청 => 응답 전송후 클라이언트 연결 종료
                        _status = ERROR_400;
                    }
                }
            }

            else
            {
                // 2.2 일반적인 read 상태에서 처리
                if (_body.size() == _contentLength)
                {
                    // 정상 수신
                    // 1. 응답 생성 준비
                    // 2. 모니터링 WRITE 모드로 전환
                    _status = READ_END;
                }
                else if (_body.size() >= _contentLength)
                {
                    // 초과 수신 => 응답 전송후 클라이언트 연결 종료
                    _errMessage = std::string("");
                }
                else
                {
                    // 계속해서 수신
                }
            }
        }
    }

    return bytes_read;
}

int Client::chunkRead(void)
{
    std::string body;
    std::string::size_type pos = 0;
    std::string::size_type old_pos = 0;

    while ((pos = _body.find("\r\n", old_pos)) != std::string::npos)
    {
        body.append(old_pos, pos - old_pos);
        old_pos = pos + 2;
    }
    return 0;
}

int change(const char *hexString)
{
    char *end;

    long num = std::strtol(hexString, &end, 16);

    if (*end == 0)
    {
        std::cout << "Parsed number: " << num << std::endl; // Output: 26
    }
    else
    {
        std::cout << "Parsing failed. Remaining string: " << end << std::endl;
    }
    return 0;
}

int Client::sendProcess(void)
{
    // read => 응답 생성 => 여기는 보내는 곳.
    const std::string data = _response.getData();
    ssize_t bytes_sent = send(_socket, data.c_str(), data.length(), 0);
    std::cout << "bytes_sent :" << bytes_sent << std::endl;
    if (bytes_sent == -1)
    {
        // error 핸들링 필요
    }
    else
    {
        _response.updateSendedBytes(bytes_sent);
    }

    if (_response.getSendedBytes() >= _response.getTotalBytes())
    {
        //  전송 끝.
        return 1;
    }
    return 0;
}

int Client::readFile(const std::string &filePath)
{
    char buffer[1024];
    
    std::string content;
    
    struct stat fileStat;

    if (stat(filePath.c_str(), &fileStat) != 0)
    {
        setResponseStatus(404, std::string("Not Found"));
        return 404;
    }

    else if (!(fileStat.st_mode & S_IRUSR))
    {
        setResponseStatus(403, std::string("Forbidden"));
        return 403;
    }

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file)
    {
        setResponseStatus(500, std::string("Internal Server Error"));
        return 500;
    }

    while (!file.eof())
    {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0)
        {
            content.append(buffer, bytesRead);
        }
    }

    file.close();

    _response.setBody(content);
    _response.setHeader(std::string("Content-Length"), std::string(Util::itoa(_response.getBody().length())));
    setResponseStatus(200, std::string("OK"));
    return 200;
}

// 요청을 다 읽은 후, 생성하는 단계 => path에 해당하는 정적 파일 제공 하기
int Client::makeResponse(const std::string & filePath)
{
    // 1. http 버전 설정
    _response.setHttpVersion(std::string("1.1"));

    // 2. 바디 생성 & 컨텐츠 길이 헤더 설정
    if (!filePath.empty())
    {
        readFile(filePath);
    }
    else
    {
        _response.setHeader(std::string("Content-Length"), std::string(Util::itoa(_response.getBody().length())));
    }

    // 3. 
    
    _response.setHeader(std::string("Date"), Util::getDateString());

    _response.generateResponseData();
    
    return _response.getStatusCode();
    
}

void Client::setResponseStatus(int statusCode, const std::string &statusMessage)
{
    _response.setStatusCode(statusCode);
    _response.setStatusMessage(statusMessage);
}

int Client::checkSendBytes() const
{
    return _response.getSendedBytes() - _response.getTotalBytes();
}

void Client::cleanRequestReponse(void)
{
    // _request.clean();
    _response.clean();
}

int Client::getSocket(void) const { return _socket; }

std::string &Client::getData(void) { return _data; }