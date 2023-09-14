/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:39 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/15 02:52:39 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

static std::string			extractHttpHeader(std::string const& rawData);
static std::vector<Header>	extractHttpHeaders(std::string const& header);
static std::string			extractHttpBody(std::string const& rawData);
static unsigned int			extractContentLength(std::string const& header);
static std::string			extractTransferEncoding(std::string const& header);
static std::string			extractContentType(std::string const& header);
static std::string			extractHttpMethod(std::string const& header);
static std::string			extractRequestUrl(std::string const& header);
static std::string			extractHttpVersion(std::string const& header);
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
	updateRequestLine();
	updateHttpHeader();
	updateHttpBody();
}

Request::Request(Request const& target)
{
	*this = target;
}

Request&	Request::operator=(Request const& target)
{
	if (this != &target)
	{
//		this->_rawData = target.getRawData();
//		this->_header = target.getHttpHeader();
//		this->_headers = target.getHttpHeaders();
//		this->_body = target.getHttpBody();
//		this->_contentLength = target.getContentLength();
//		this->_transferEncoding = target.getTransferEncoding();
//		this->_contentType = target.getContentType();
//		this->_method = target.getHttpMethod();
//		this->_requestUrl = target.getRequestUrl();
//		this->_httpVersion = target.getHttpVersion();
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

std::string const&	Request::getHttpHeader(void) const
{
	return this->_header;
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
 * @brief updateRequestLine
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Request Line과 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::updateRequestLine(void)
{
	this->_method = extractHttpMethod(this->_rawData);
	this->_requestUrl = extractRequestUrl(this->_rawData);
	this->_httpVersion = extractHttpVersion(this->_rawData);
}

/**
 * @brief updateHttpHeader
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Header와 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::updateHttpHeader(void)
{
	this->_header = extractHttpHeader(this->_rawData);
	this->_headers = extractHttpHeaders(this->_header);
	this->_contentLength = extractContentLength(this->_header);
	this->_transferEncoding = extractTransferEncoding(this->_header);
	this->_contentType = extractContentType(this->_header);
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
	for (std::vector<Header>::iterator it = this->_headers.begin();
		it != this->_headers.end(); ++it)
	{
		if (it->key == key)
		{
			it->value = newValue;
			break;
		}
	}
}

/**
 * @brief updateHttpBody
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Body와 관련된 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::updateHttpBody(void)
{
	this->_body = extractHttpBody(this->_rawData);
}

void	Request::handleChunkedBody(void)
{
	if (this->_transferEncoding == "chunked")
	{
		this->_body = extractChunkedBody(this->_body);
		this->_contentLength = static_cast<unsigned int>(this->_body.size());
		updateHeaderValue
		("content-length", std::to_string(this->_contentLength));
	}
}

void	Request::handleMultipartBody(void)
{
	std::string	boundary;
	std::size_t	pos;

	if (this->_contentType.find("multipart/form-data;") != std::string::npos)
	{
		if ((pos = this->_contentType.find("boundary=")) != std::string::npos)
		{
			boundary = this->_contentType.substr(pos + 9);
			this->_contents = extractMultipartBody(this->_body, boundary);
		}
	}
}

bool	Request::isLastChunk(void) const
{
	std::size_t	pos = this->_body.rfind("\r\n0\r\n");
	std::string	line;
	std::string	lastChunkSize;
	std::string	lastCrlf;

	if (pos != std::string::npos)
	{
		pos += 2;
		line = this->_body.substr(pos);
		if (line.length() < 5)
			return false;
		lastChunkSize = line.substr(0, 3);
		lastCrlf = line.substr(line.length() - 2, 2);
		if ((lastChunkSize == "0\r\n" && lastCrlf == "\r\n"))
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
	this->_header.clear();
	this->_headers.clear();
	this->_body.clear();
	this->_contentLength = 0;
	this->_transferEncoding.clear();
	this->_contentType.clear();
	this->_method.clear();
	this->_requestUrl.clear();
	this->_httpVersion.clear();
}

/**
 * Helper Function
 */

static std::string	extractHttpHeader(std::string const& rawData)
{
	std::size_t	headerPos = rawData.find(DOUBLE_CRLF);

	if (headerPos != std::string::npos)
		return rawData.substr(0, headerPos + 4);
	return std::string();
}

static std::vector<Header>	extractHttpHeaders(std::string const& header)
{
	std::vector<Header>	headers;
	std::size_t			pos;
	std::size_t			oldPos;
	std::size_t			colPos;
	std::string			line;

	oldPos = 0;
	while ((pos = header.find(CRLF, oldPos)) != std::string::npos)
	{
		line = header.substr(oldPos, pos - oldPos);
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

static unsigned int	extractContentLength(std::string const& header)
{
	std::string	value;
	std::size_t	startPos;
	std::size_t	endPos;

	startPos = header.find("Content-Length:");
	if (startPos != std::string::npos)
	{
		startPos += 15;
		endPos = header.find(CRLF, startPos);
		if (endPos != std::string::npos)
		{
			value = header.substr(startPos, endPos - startPos);
			return static_cast<unsigned int>(atoi(value.c_str()));
		}
	}
	return 0;
}

static std::string	extractTransferEncoding(std::string const& header)
{
	std::string	value;
	std::size_t	startPos;
	std::size_t	endPos;

	startPos = header.find("Transfer-Encoding:");
	if (startPos != std::string::npos)
	{
		startPos += 18;
		endPos = header.find(CRLF, startPos);
		if (endPos != std::string::npos)
		{
			value = Util::lrtrim(header.substr(startPos, endPos - startPos));
			return value;
		}
	}
	return std::string();
}

static std::string	extractContentType(std::string const& header)
{
	std::string	value;
	std::size_t	startPos;
	std::size_t	endPos;

	startPos = header.find("Content-Type:");
	if (startPos != std::string::npos)
	{
		startPos += 13;
		endPos = header.find(CRLF, startPos);
		if (endPos != std::string::npos)
		{
			value = Util::lrtrim(header.substr(startPos, endPos - startPos));
			return value;
		}
	}
	return std::string();
}

static std::string	extractHttpMethod(std::string const& header)
{
	std::string	method;
	std::size_t	findPos;

	findPos = header.find(' ');
	if (findPos != std::string::npos)
	{
		method = header.substr(0, findPos);
		if (method == "CONNECT" || method == "DELETE" || method == "GET"
			|| method == "HEAD" || method == "OPTIONS" || method == "POST"
			|| method == "PUT" || method == "TRACE")
			return method;
	}
	return std::string();
}

static std::string	extractRequestUrl(std::string const& header)
{
	std::size_t	findPos1;
	std::size_t	findPos2;

	findPos1 = header.find(' ');
	if (findPos1 == std::string::npos)
		return std::string();
	findPos2 = header.find(' ', ++findPos1);
	if (findPos2 != std::string::npos)
		return header.substr(findPos1, findPos2 - findPos1);
	return std::string();
}

static std::string	extractHttpVersion(std::string const& header)
{
	std::string	httpVersion;
	std::size_t	findPos1;
	std::size_t	findPos2;
	std::size_t	endPos;

	findPos1 = header.find(' ');
	if (findPos1 == std::string::npos)
		return std::string();
	findPos2 = header.find(' ', ++findPos1);
	if (findPos2++ != std::string::npos)
	{
		endPos = header.find(CRLF);
		httpVersion = header.substr(findPos2, endPos - findPos2);
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
			std::cout << "Malformed: " << std::endl;
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

	// 이게 맞나...
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
//		std::cout << "[" << line << "]" << std::endl;
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
		line = body.substr(oldPos, pos - oldPos);
//		std::cout << "[" << line << "]" << std::endl;
		entry.data += line;
		contents.push_back(entry);
		oldPos = pos;
	}
	return contents;
}
