/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:39 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/19 18:27:01 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

static std::vector<Header>	extractHttpHeaders(std::string const& rawData);
static std::string			extractHttpBody(std::string const& rawData);
static std::string			extractHttpMethod(std::string const& rawdata);
static std::string			extractRequestUrl(std::string const& rawData);
static std::string			extractHttpVersion(std::string const& rawData);
static std::string			extractChunkedBody(std::string const& body);
static std::vector<Content>	extractMultipartBody
(std::string const& body, std::string& boundary);

/**
 * Constructor & Destroctor
 */

Request::Request(void) : _contentLength(0) {}

// header -> body
Request::Request(std::string const& data)
{
	this->_rawData = data;
	readRequestLine();
	readHttpHeader();
	readHttpBody();
}

Request::Request(Request const& target)
{
	*this = target;
}

// 현재 얕은 복사로 되어있음
Request&	Request::operator=(Request const& target)
{
	if (this != &target)
	{
		this->_rawData = target.getRawData();
		this->_headers = target.getHttpHeaders();
		this->_body = target.getHttpBody();
		this->_contentLength = target.getContentLength();
		this->_transferEncoding = target.getTransferEncoding();
		this->_contentType = target.getContentType();
		this->_contents = target.getContents();
		this->_method = target.getHttpMethod();
		this->_requestUrl = target.getRequestUrl();
		this->_httpVersion = target.getHttpVersion();
	}
	return *this;
}

Request::~Request(void) {}

/**
 * Getter & Setter
 */

std::string const&	Request::getRawData(void) const
{
	return this->_rawData;
}

void	Request::setRawData(std::string const rawData)
{
	this->_rawData = rawData;
}

std::vector<Header> const&	Request::getHttpHeaders(void) const
{
	return this->_headers;
}

std::string const&	Request::getHttpBody(void) const
{
	return this->_body;
}

unsigned int	Request::getContentLength(void) const
{
	return this->_contentLength;
}

std::string const&	Request::getTransferEncoding(void) const
{
	return this->_transferEncoding;
}

std::string const&	Request::getContentType(void) const
{
	return this->_contentType;
}

std::vector<Content> const&	Request::getContents(void) const
{
	return this->_contents;
}

std::string const&	Request::getHttpMethod(void) const
{
	return this->_method;
}

std::string const&	Request::getRequestUrl(void) const
{
	return this->_requestUrl;
}

std::string const&	Request::getHttpVersion(void) const
{
	return this->_httpVersion;
}

/**
 * Member Function
 */

/**
 * @brief readRequestLine
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Request Line과 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::readRequestLine(void)
{
	this->_method = extractHttpMethod(this->_rawData);
	this->_requestUrl = extractRequestUrl(this->_rawData);
	this->_httpVersion = extractHttpVersion(this->_rawData);
}

/**
 * @brief readHttpHeader
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Header와 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::readHttpHeader(void)
{
	this->_headers = extractHttpHeaders(this->_rawData);
	this->_contentLength = static_cast<unsigned int>
		(atoi(findHeaderValue("Content-Length").c_str()));
	this->_transferEncoding = findHeaderValue("Transfer-Encoding");
	this->_contentType = findHeaderValue("Content-Type");
}

/**
 * @brief readHttpBody
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Body와 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::readHttpBody(void)
{
	this->_body = extractHttpBody(this->_rawData);
}

/**
 * @brief updateHeaderValue
 *
 * 인자로 key와 value를 받아, headers에 저장된 구조체를 순회하며,
 * 일치하는 key 필드를 가진 구조체의 value 필드를 업데이트합니다.
 *
 * @param std::string const& key, std::string const& newValue;
 * @return void
 */
void	Request::updateHeaderValue
		(std::string const& key, std::string const& newValue)
{
	std::vector<Header>&	headers = this->_headers;

	for (std::vector<Header>::iterator hit = headers.begin();
		hit != headers.end(); ++hit)
	{
		if (hit->key == Util::toLowerCase(key))
		{
			hit->value = newValue;
			break;
		}
	}
}

/**
 * @brief FindHeaderValue
 *
 * 인자로 key를 받아, headers에 저장된 구조체를 순회하며,
 * 일치하는 key 필드를 가진 구조체의 value 필드를 복사하여 가져옵니다.
 * 찾는 값이 없는 경우, 빈 문자열을 반환합니다.
 *
 * @param std::string const& key;
 * @return std::string value;
 */
std::string	Request::findHeaderValue(std::string const& key)
{
	std::vector<Header>&	headers = this->_headers;

	for (std::vector<Header>::iterator hit = headers.begin();
		hit != headers.end(); ++hit)
	{
		if (hit->key == Util::toLowerCase(key))
			return hit->value;
	}
	return std::string();
}

/**
 * @brief handleChunkedBody
 *
 * chunked 형식으로 온 _body의 문자열을
 * 일반 HTTP Body 형식으로 재구성합니다.
 * 관련된 header의 데이터도 업데이트합니다.
 *
 * @param void
 * @return void
 */
void	Request::handleChunkedBody(void)
{
	if (this->_transferEncoding.find("chunked") != std::string::npos)
	{
		this->_body = extractChunkedBody(this->_body);
		this->_contentLength = static_cast<unsigned int>(this->_body.size());
		updateHeaderValue(
			"Content-Length",
			std::to_string(this->_contentLength)
		);
	}
}

/**
 * @brief handleMultipartBody
 *
 * Multipart 형식으로 온 _body 문자열을
 * 요청대로 처리하여 _contents에 저장합니다.
 *
 * @param void
 * @return void
 */
void	Request::handleMultipartBody(void)
{
	std::string	boundary;
	std::size_t	pos;

	// need validation check
	// 오류인 경우 어떻게 catch할까?
	if (this->_contentType.find("multipart/form-data;") != std::string::npos)
	{
		if ((pos = this->_contentType.find("boundary=")) != std::string::npos)
		{
			boundary = this->_contentType.substr(pos + 9);
			this->_contents = extractMultipartBody(this->_body, boundary);
		}
	}
}

/**
 * @brief isLastChunk
 *
 * chunked 형식으로 저장된 _body 문자열에
 * 끝까지 전송되었음을 나타내는 last chunk가
 * 올바르게 들어있는지 확인합니다.
 *
 * @param void
 * @return bool
 */
bool	Request::isLastChunk(void) const
{
	std::size_t	pos;
	std::size_t	lastChunkPos;
	std::string	line;
	std::string	lastChunk;
	std::string	lastTwoChars;

	if ((pos = this->_body.rfind(CRLF)) == std::string::npos
		|| (pos = this->_body.rfind(CRLF, --pos)) == std::string::npos)
		return false;
	lastChunkPos = pos;
	lastTwoChars = this->_body.substr(this->_body.length() - 2);
	if ((pos = this->_body.rfind(CRLF, --pos)) != std::string::npos)
	{
		pos += 2;
		line = this->_body.substr(pos);
		lastChunk = Util::removeDuplicate
					(line.substr(0, lastChunkPos - pos));
		if ((lastChunk == "0") && (lastTwoChars == CRLF))
			return true;
	}
	else
	{
		line = this->_body;
		lastChunk = Util::removeDuplicate
					(line.substr(0, lastChunkPos));
		if ((lastChunk == "0") && (lastTwoChars == CRLF))
			return true;
	}
	return false;
}

/**
 * @brief resetRequest
 *
 * Reqeust를 초기화합니다.
 *
 * @param void
 * @return void
 */
void	Request::resetRequest(void)
{
	this->_rawData.clear();
	this->_headers.clear();
	this->_body.clear();
	this->_contentLength = 0;
	this->_transferEncoding.clear();
	this->_contentType.clear();
	this->_contents.clear();
	this->_method.clear();
	this->_requestUrl.clear();
	this->_httpVersion.clear();
}

/**
 * Helper Function
 */

static std::vector<Header>	extractHttpHeaders(std::string const& rawData)
{
	std::vector<Header>	headers;
	std::size_t			pos;
	std::size_t			oldPos;
	std::size_t			colPos;
	std::string			line;

	if (rawData.find(DOUBLE_CRLF) == std::string::npos)
		return headers;
	oldPos = 0;
	while ((pos = rawData.find(CRLF, oldPos)) != std::string::npos)
	{
		line = rawData.substr(oldPos, pos - oldPos);
		colPos = line.find(":");
		if (colPos != std::string::npos)
		{
			Header	entry;
			entry.key = Util::toLowerCase(line.substr(0, colPos));
			entry.value = Util::lrtrim(line.substr(colPos + 1));
			headers.push_back(entry);
		}
		oldPos = pos + 2;
	}
	return headers;
}

static std::string	extractHttpBody(std::string const& rawData)
{
	std::size_t	bodyPos = rawData.find(DOUBLE_CRLF);

	if (bodyPos != std::string::npos)
		return rawData.substr(bodyPos + 4);
	return std::string();
}

static std::string	extractHttpMethod(std::string const& rawData)
{
	std::string	method;
	std::size_t	findPos;

	findPos = rawData.find(' ');
	if (findPos != std::string::npos)
	{
		method = rawData.substr(0, findPos);
		if (method == "DELETE" || method == "GET"
			|| method == "HEAD" || method == "POST" || method == "PUT")
			return method;
	}
	return std::string();
}

static std::string	extractRequestUrl(std::string const& rawData)
{
	std::size_t	findPos1;
	std::size_t	findPos2;

	findPos1 = rawData.find(' ');
	if (findPos1 == std::string::npos)
		return std::string();
	findPos2 = rawData.find(' ', ++findPos1);
	if (findPos2 != std::string::npos)
		return rawData.substr(findPos1, findPos2 - findPos1);
	return std::string();
}

static std::string	extractHttpVersion(std::string const& rawData)
{
	std::string	httpVersion;
	std::size_t	findPos1;
	std::size_t	findPos2;
	std::size_t	endPos;

	findPos1 = rawData.find(' ');
	if (findPos1 == std::string::npos)
		return std::string();
	findPos2 = rawData.find(' ', ++findPos1);
	if (findPos2++ != std::string::npos)
	{
		endPos = rawData.find(CRLF);
		httpVersion = rawData.substr(findPos2, endPos - findPos2);
		if (httpVersion == "HTTP/1.1" || httpVersion == "HTTP/1.0")
			return httpVersion;
	}
	return std::string();
}

static std::size_t	readChunkSize(std::string& line)
{
	std::size_t				chunkSize = 0;
	std::string::iterator	it;
	char					hexChar;

	for (it = line.begin(); it != line.end(); ++it)
	{
		hexChar = *it;

		if (std::isspace(hexChar))
			continue;
		if (hexChar >= '0' && hexChar <= '9')
			chunkSize = chunkSize * 16
						+ static_cast<unsigned int>(hexChar - '0');
		else if (hexChar >= 'a' && hexChar <= 'f')
			chunkSize = chunkSize * 16
				+ static_cast<unsigned int>(hexChar - 'a' + 10);
		else if (hexChar >= 'A' && hexChar <= 'F')
			chunkSize = chunkSize * 16
				+ static_cast<unsigned int>(hexChar - 'A' + 10);
		else {
			// Malformed Data
			// 어떻게 처리할까?
			std::cout << "Malformed: " << hexChar << std::endl;
		}
	}
	return chunkSize;
}

static std::string	extractChunkedBody(std::string const& body)
{
	std::string	chunkedBody;
	std::string	line;
	std::size_t	chunkSize;
	std::size_t	pos;
	std::size_t	oldPos = 0;

	while ((pos = body.find(CRLF, oldPos)) != std::string::npos)
	{
		line = body.substr(oldPos, pos - oldPos);
		chunkSize = readChunkSize(line);
		if (chunkSize == 0)
			break;
		oldPos = pos + 2;
		chunkedBody.append(body, oldPos, chunkSize);
		oldPos += chunkSize + 2;
	}
	return chunkedBody;
}

static std::vector<Content>	extractMultipartBody
(std::string const& body, std::string& boundary)
{
	std::vector<Content>		contents;
	std::vector<std::string>	tokens;
	std::vector<std::string>	pair;
	std::string					line;
	std::size_t					pos;
	std::size_t					oldPos = 0;
	bool						flag = true;

	// boundary validation check
	boundary = "--" + boundary;
	while (((pos = body.find(boundary, oldPos)) != std::string::npos)
			&& flag == true)
	{
		Content	entry;
		pos += boundary.length();
		line = body.substr(oldPos, pos - oldPos + 2);
//		std::cout << "[" << line << "]" << std::endl;
		if (line == boundary + "--")
		{
			break;
		}
		oldPos = pos + 2;
		pos = body.find(CRLF, oldPos);
		if (pos == std::string::npos)
		{
			std::cout << "Malformed body data" << std::endl;
			break;
		}
		line = body.substr(oldPos, pos - oldPos);
//		std::cout << "[" << line << "]" << std::endl;
		if (line.find("Content-Disposition") != std::string::npos)
		{
			tokens = Util::splitString(line, ';');
			for (std::vector<std::string>::iterator it = tokens.begin();
				it != tokens.end(); ++it)
			{
				pair = Util::splitString(*it, '=');
				if (pair.size() < 1)
				{
					std::cout << "Malformed Content Disposition" << std::endl;
					flag = false; // throw error
					break;
				}
				if (Util::lrtrim(pair[0]) == "name")
					entry.name = Util::lrdtrim(pair[1], "\"");
				else if (Util::lrtrim(pair[0]) == "filename")
					entry.filename = Util::lrdtrim(pair[1], "\"");
			}
		}
		oldPos = pos + 2;
		pos = body.find(CRLF, oldPos);
		if (pos == std::string::npos)
		{
			std::cout << "Malformed body data" << std::endl;
			break;
		}
		line = body.substr(oldPos, pos - oldPos);
//		std::cout << "TYPE : [" << line << "]" << std::endl;
		if (line.find("Content-Type") != std::string::npos)
		{
			pair = Util::splitString(line, ':');
			if (pair.size() < 2)
			{
				std::cout << "Malformed body data" << std::endl;
				flag = false;
				break;
			}
			entry.type = Util::lrtrim(pair[1]);
			oldPos = pos + 2;
		}
		pos = body.find(boundary, oldPos);
		if (pos == std::string::npos)
		{
			std::cout << "Malformed body data" << std::endl;
			break;
		}
		oldPos += 2;
		line = body.substr(oldPos, pos - oldPos - 2);
//		std::cout << "DATA : [" << line << "]" << std::endl;
		entry.data += line;
		contents.push_back(entry);
		oldPos = pos;
	}
	return contents;
}
