#include <string>
#include <iostream>

static const std::string CRLF("\r\n");

bool hexToDecimal(const std::string &hexStr, size_t &decimalValue)
{
    char *end;
    errno = 0; // errno를 0으로 설정하여 이전의 오류를 지웁니다.
    size_t tempValue = std::strtoul(hexStr.c_str(), &end, 16);

    // 변환에 실패하거나 값이 int의 범위를 벗어나면 false를 반환합니다.
    if (errno == ERANGE || *end == '\0')
    {
        return false;
    }

    decimalValue = tempValue;
    return true;
}

bool hexToDecimalPositive(const std::string &hexStr, size_t &decimalValue)
{
    size_t tempValue;
    if (!hexToDecimal(hexStr, tempValue) || tempValue < 0)
    {
        return false;
    }
    decimalValue = tempValue;
    return true;
}

bool isLastChunk(const std::string &data)
{
    size_t pos = data.find_first_not_of("0");

    // "CRLF" ...
    if (pos == 0)
    {
        return false;
    }
    // 1*("0")
    if (pos != std::string::npos)
    {
        return false;
    }
    // 1*("0") "CRLF"
    if (data.substr(pos, pos + 2).compare(CRLF) == 0)
    {
        return true;
    }
    return false;
}

std::size_t tryReadChunk(const std::string &rawData)
{
    size_t size;
    size_t crlfPos1 = rawData.find(CRLF);

    if (crlfPos1 == 0)
    {
        throw std::invalid_argument("None hex digit!");
    }

    if (crlfPos1 == std::string::npos)
    {
        // 아직 CRLF가 없어서 파싱 중단
        return 0;
    }

    std::string hexStr = rawData.substr(0, crlfPos1);

    if (!hexToDecimal(hexStr, size))
    {
        throw std::invalid_argument("Invalid hex digit");
    }

    if (rawData.size() < crlfPos1 + 2 + size + 2)
    {
        // 아직 전체 구조가 생기기 위한 최소 길이 충족 못 해서 파싱 중단
        return 0;
    }

    size_t crlfPos2 = crlfPos1 + 2 + size;

    if (rawData.substr(crlfPos2, crlfPos2 + 2).compare(CRLF) != 0)
    {
        throw std::invalid_argument("Invalid OCTET Line");
    }

    return size;
}

int Client::chunkRead(void)
{
    std::string data;
    std::string chunkOctet;
    std::size_t size;

    std::string rawData = _request.getRawData();

    try
    {
        while (!isLastChunk(rawData))
        {
            // data, chunkOctet 모두 변화함 asdasd
            size = tryReadChunk(rawData); // throw std::invalid_argument
            if (size == 0)
                return; // 아직 파싱할만큼 데이터가 없음
            _chunkOctetData.append(rawData, size);
            _request.setRawData(rawData.substr(size));
            if (_chunkOctetData.size() > _bodyLimit)
            {
                throw 413;
            }
        }
        // 라스트 청크발견
        _status = READ_BODY;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << e.what() << std::endl;
        throw 400;
    }
    return 0;
}