#include "Client.hpp"

Client::Client(int serverSocket, Webserver *ws, Server *s, EventHandler *e)
    : _ws(ws), _server(s), _eventHandler(e), _contentLength(0), _status(0), _ischunk(0), _erron(0), _defaultBodyNeed(0)
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
    (void)_server;
    (void)_eventHandler;
}

Client::~Client(void) {}

int Client::notCgiGetProcess(const std::string &filepath)
{
    try
    {
        std::string fileData = File::getFile(filepath);
        _response.setBody(fileData);
        _response.setStatusCode(200);
        _erron = 0;
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1; // _erron 삭제 해도 될 수도
        _defaultBodyNeed = 1; // 에러는 기본 바디 필요
    }
    return _status;
}

int Client::notCgiPostProcess(const std::string &filepath, const std::string &body)
{
    try
    {
        File::uploadFile(filepath, body);
        _response.setStatusCode(201); // created
        _erron = 0;
        _defaultBodyNeed = 1; // Post는 바디 콘텐츠 X
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return _status;
}

int Client::notCgiDeleteProcess(const std::string &filepath)
{
    try
    {
        File::deleteFile(filepath);
        _response.setStatusCode(200);
        _erron = 0;
        _defaultBodyNeed = 1; // Delete는 바디 콘텐츠 X
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return _status;
}

int Client::readDefaultErrorFile(const std::string &filepath)
{
    try
    {
        _body = File::getFile(filepath);
    }
    catch (int statusCode)
    {
        _body.clear();
        _status = statusCode;
        _erron = 1;
    }
    return _status;
}

int Client::afterRead(void)
{
    const std::string GET_METHOD("GET");
    const std::string POST_METHOD("POST");
    const std::string DELETE_METHOD("DELETE");

    std::cout << RED << "afterRead()" << RESET << std::endl;

    // HTTP Request가 완성 되었다. => method && path
    // 1. GET /2MB.jpeg
    std::string tempMethod("GET");
    // std::string tempMethod("POST");
    // std::string tempMethod("DELETE");

    // 2. location과 매칭해야한다.

    // 3. CGI vs Not CGI
    bool tempCgi = false;
    if (!tempCgi)
    {
        if (tempMethod.compare(GET_METHOD) == 0)
        {
            std::cout << BLUE << "GET_METHOD()" << RESET << std::endl;
            std::string filepath("./2MB.jpeg");
            notCgiGetProcess(filepath);
        }
        else if (tempMethod.compare(POST_METHOD) == 0)
        {
            std::cout << BLUE << "POST_METHOD()" << RESET << std::endl;
            const std::string filepath("./2MB_UPLOADED.jpeg");
            std::string tempBody = File::getFile("./2MB.jpeg");
            notCgiPostProcess(filepath, tempBody);
        }
        else if (tempMethod.compare(DELETE_METHOD) == 0)
        {
            std::cout << BLUE << "DELETE_METHOD()" << RESET << std::endl;
            const std::string filepath("./2MB_UPLOADED.jpeg");
            notCgiDeleteProcess(filepath);
        }
        else
        {
            // 에러
        }
        // 치명적인 에러 => 응답 필요 X, 클라이언트 삭제
        // 평범한 에러 => 에러 응답 생성 필요 O => 아래로 진행
    }
    else
    {
        // CGI 처리
        if (true)
        {
            // 성공시 kqueue 모니터링 등록후 이동
            return 0;
        }
        // (실패 == 에러) => 에러 응답하면 됨 => 아래로 진행
    }

    if (_defaultBodyNeed)
    {
       _response.setBody(createDefaultBody(_status));
    }
    
    makeResponseData();

    return 0;
}

std::string Client::createDefaultPage(int statusCode)
{
    const std::string defaultPage("/err.html");
    std::string html;

    try
    {
        if (!defaultPage.empty())
        {
            const std::string &filepath = std::string(".") + defaultPage;
            html = File::getFile(filepath);
            
        } else {
            html = createDefaultBody(statusCode);
        }
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        html = createDefaultBody(statusCode);
    }

    return html;
}

std::string Client::createDefaultBody(int statusCode)
{
    const std::string SP(" ");
    std::string body;

    std::string statusCodeComment =
        Util::ft_itoa(statusCode) + SP + Util::getStatusCodeMessage(statusCode);

    body += "<html>";
    body += "<head><title>" + statusCodeComment + "</title></head>";
    body += "<body>";
    body += "<center><h1>" + statusCodeComment + "</h1></center>";
    body += "<hr><center>webserv/hyeonjanX</center>";
    body += "</body>";
    body += "</html>";

    return body;
}

int Client::readProcess(void)
{
    // 1. 데이터 읽기 => 청크 모드 ? 특별 처리 : 일반처리

    // ================ FOR TEST =============
    std::vector<char> buffer(100000);
    ssize_t bytes_read = read(_socket, buffer.data(), buffer.size());

    if (bytes_read > 0)
    {

        std::cout << "========== 입력받은 데이터 (" << bytes_read << ")==========" << std::endl;
        std::cout << std::string(buffer.begin(), buffer.end()) << std::endl;
        std::cout << "------------------------------" << std::endl;

        buffer.resize(bytes_read);                  // 실제로 읽은 데이터만큼만 크기를 조절
        _data.append(buffer.begin(), buffer.end()); // vector의 데이터를 string에 더함
    }
    // GET 요청 => 파일 읽어 리턴.

    // 1. GET && no cgi
    // makeResponse("./large.txt");
    // makeResponse("./2mb_image.jpeg");
    // _eventHandler->turnOnWrite(_socket);
    // return bytes_read;

    _eventHandler->turnOffRead(_socket);
    afterRead();
    // 1. http 버전 설정
    _response.setHttpVersion(std::string("1.1"));
    _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));
    _response.setHeader(std::string("Date"), Util::getDateString());
    _response.generateResponseData();
    _eventHandler->turnOnWrite(_socket);
    return 0;

    int __statusCode;
    try
    {
        // std::string fileData = File::getFile("./large.txt");
        std::string fileData = File::getFile("./miyeon.jpeg");
        _response.setBody(fileData);
        __statusCode = 200;
    }
    catch (int statusCode)
    {
        __statusCode = statusCode;
    }
    // setResponseStatus(__statusCode, Util::getStatusCodeMessage(__statusCode));
    tempMakeResponseByStatusCode(__statusCode);
    _eventHandler->turnOnWrite(_socket);
    return 0;

    // 2. POST && no cgi
    std::cout << "data.size(): " << _data.size() << std::endl;

    if (buffer.size() > 10000)
    {
        try
        {
            std::string filepath("./upload.txt");
            File::writeFile(filepath, _data);
            __statusCode = 201;
        }
        catch (int statusCode)
        {
            __statusCode = statusCode;
        }

        setResponseStatus(__statusCode, Util::getStatusCodeMessage(201));
        tempMakeResponseByStatusCode(__statusCode);
        _eventHandler->switchToWriteState(_socket);
        return 0;
    }
    return 0;

    // ================ FOR TEST END ====================

    // char buffer[1024];
    // ssize_t bytes_read = read(_socket, buffer, 1024);

    if (bytes_read == -1)
    {
        std::cerr << "Fail to read(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(_socket);
        return 0;
    }

    _data.append(buffer.begin(), buffer.end());

    if (_status == READ_REQUESTLINE)
    {
        readRequestLine();
    }
    if (_status == READ_HEADER)
    {
        readHeader();
    }
    if (_status == READ_BODY)
    {
        readBody();
    }

    if (_status == BODY_LIMIT_OVER || _status == BODY_SIZE_OVER)
    {
        // 오류
        makeResponse("./404.html");
        _eventHandler->switchToWriteState(_socket);
    }
    if (_status == READ_END)
    {
        // 수신 완료
        makeResponse("./lorem_ipsum.txt");
        _eventHandler->switchToWriteState(_socket);
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

// 현재 리턴값을 활용하고 있진 않음
// EVFILT_WRITE 이벤트에 의해 트리거 되는 곳.
// send 호출후, 모두 보냈는지 확인할 수 있는 값을 리턴,
// send 실패시 throw 가능
int Client::sendProcess(void)
{
    // std::cout << RED << "0: " << _response.getDataLength() << RESET << std::endl;
    std::cout << GREEN << " ------- sendProcess ------- " << RESET << std::endl;

    ssize_t bytes_sent = send(_socket, _response.getData().c_str(), _response.getDataLength(), 0);

    if (bytes_sent == -1)
    {
        // MSG_NOSIGNAL가 없어서 signal(SIGPIPE, SIG_IGN);를 통해서 SIGPIPE가 들어와도 프로세스가 종료되지 않도록.
        // 클라이언트에서 먼저 종료시: Broken pipe
        std::cerr << "Fail to send(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(_socket);
        return 0;
    }

    // std::cout << RED << "1" << RESET << std::endl;

    _response.updateData(bytes_sent);
    _response.updateSendedBytes(bytes_sent);

    std::cout << "send: " << bytes_sent << " bytes, (" << _response.getSendBytes() << "/" << _response.getTotalBytes() << ")" << std::endl;

    if (checkSendBytes() >= 0)
    {
        // 상태를 바꾼다거나 할 수도 있음
        _eventHandler->switchToReadState(_socket);
        cleanRequestReponse();
    }

    return 0;
    // return checkSendBytes();
}

int Client::tempMakeResponseByStatusCode(int statusCode)
{
    // 1. http 버전 설정
    _response.setHttpVersion(std::string("1.1"));

    // 2. 디폴트 바디 생성
    if (_response.getBody().empty())
    {
        _response.setBody(Util::ft_itoa(statusCode) + Util::getStatusCodeMessage(statusCode));
    }

    // 3. 콘텐츠 랭스 설정
    _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));

    // 4. Date 설정
    _response.setHeader(std::string("Date"), Util::getDateString());

    _response.setStatusCode(statusCode);
    _response.setStatusMessage(Util::getStatusCodeMessage(statusCode));
    _response.generateResponseData();

    return _response.getStatusCode();
}

void Client::makeResponseData(void)
{
    _response.setHttpVersion(std::string("1.1"));
    _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));
    _response.setHeader(std::string("Date"), Util::getDateString());
    _response.generateResponseData();
}

// 요청을 다 읽은 후, 생성하는 단계 => path에 해당하는 정적 파일 제공 하기
int Client::makeResponse(const std::string &filePath)
{
    // 1. http 버전 설정
    _response.setHttpVersion(std::string("1.1"));

    // 2. 바디 생성 & 컨텐츠 길이 헤더 설정
    if (!filePath.empty())
    {
        std::string data = File::getFile(filePath);
        _response.setBody(data);
    }
    else
    {
        _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));
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

/** return (보낸바이트 - 보내야할바이트), 0 >= 0 이면 송신 끝을 의미 **/
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

void Client::readRequestLine(void)
{
    // 1. 헤더 끝까지 읽기
    size_t pos = _data.find("\r\n");
    if (pos != std::string::npos)
    {
        _requestLine.append(_data, pos + 2);
        _data = _data.substr(pos + 2);
        _status = READ_HEADER;
    }
}

// 헤더 크기 제한 어딘가에 적용하기?
void Client::readHeader(void)
{
    std::string::size_type pos = 0;
    std::string::size_type old_pos = 0;

    while ((pos = _data.find("\r\n", old_pos)) != std::string::npos)
    {
        if (pos == 0)
        {
            _data = _data.substr(pos + 2);
            _status = READ_BODY;
            // 헤더 분석
            return;
        }

        std::string line = _data.substr(old_pos, pos - old_pos);

        _header.append(line);

        // std::string::size_type colon_pos = line.find(":");

        //  if (colon_pos != std::string::npos)
        // {
        //     std::string name = line.substr(0, colon_pos);
        //     std::string value = line.substr(colon_pos + 1);
        //     _header2.push_back(std::make_pair(name, value));
        // }
        old_pos = pos + 2;
    }

    if (old_pos)
    {
        _data = _data.substr(old_pos);
    }
}

// MUST: 바디 크기 제한
void Client::readBody(void)
{
    // 1. data 읽어들이기
    if (_ischunk)
    {
        // 2.1 chuncked 처리
        size_t pos = _data.find("\r\n\r\n");
        if (pos != std::string::npos)
        {
            // 끝을 찾음
            if (pos + 4 == _data.size())
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
        // 청크 모드가 아니면 바디에 전부 추가하면 될듯?
        _body.append(_data);
        _data.clear();
    }

    // 2. 바디 제한 길이 확인
    if (_body.size() >= _bodyLimit)
    {
        _status = BODY_LIMIT_OVER;
        return;
    }

    // 3. 본문 길이 확인
    if (_body.size() > _contentLength)
    {
        _status = BODY_SIZE_OVER;
    }
    else if (_body.size() == _contentLength)
    {
        _status = READ_END;
    }
    else
    {
        // 계속해서 수신
    }
}