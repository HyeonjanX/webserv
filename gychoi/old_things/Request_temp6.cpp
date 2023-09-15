/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:39 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/15 23:13:08 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request_temp6.hpp"

static std::string	extractHttpHeader(std::string const& rawData);
static std::string	extractHttpBody(std::string const& rawData);
static unsigned int	extractContentLength(std::string const& header);
static std::string	extractTransferEncoding(std::string const& header);
static std::string	extractHttpMethod(std::string const& header);
static std::string	extractRequestUrl(std::string const& header);
static std::string	extractHttpVersion(std::string const& header);
static std::string	extractChunkedBody(std::string const& body);

/**
 * Constructor & Destroctor
 */

Request::Request(void) : _contentLength(0) {}

Request::Request(std::string const& data)
{
	this->_rawData = data;
	this->_header = extractHttpHeader(this->_rawData);
	this->_body = extractHttpBody(this->_rawData);
	this->_contentLength = extractContentLength(this->_header);
	this->_transferEncoding = extractTransferEncoding(this->_header);
	this->_method = extractHttpMethod(this->_rawData);
	this->_requestUrl = extractRequestUrl(this->_rawData);
	this->_httpVersion = extractHttpVersion(this->_rawData);
}

Request::Request(Request const& target)
{
	*this = target;
}

Request&	Request::operator=(Request const& target)
{
	if (this != &target)
	{
		this->_rawData = target.getRawData();
		this->_header = target.getHttpHeader();
		this->_body = target.getHttpBody();
		this->_contentLength = target.getContentLength();
		this->_transferEncoding = target.getTransferEncoding();
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

std::string const&	Request::getHttpHeader(void) const
{
	return this->_header;
}

std::map<std::string, std::string> const&	Request::getHttpHeaders(void) const
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
 * @brief updateRequest
 *
 * _rawData에 저장된 HTTP 문자열을 바탕으로
 * Request 멤버 변수를 모두 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::updateRequest(void)
{
	this->_header = extractHttpHeader(this->_rawData);
	this->_body = extractHttpBody(this->_rawData);
	this->_contentLength = extractContentLength(this->_rawData);
	this->_method = extractHttpMethod(this->_rawData);
	this->_requestUrl = extractRequestUrl(this->_rawData);
	this->_httpVersion = extractHttpVersion(this->_rawData);
}

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
	std::size_t	pos;
	std::size_t	oldPos;
	std::size_t	colPos;
	std::string	line;
	std::string	key;
	std::string	value;

	this->_header = extractHttpHeader(this->_rawData);
	this->_contentLength = extractContentLength(this->_header);
	this->_transferEncoding = extractTransferEncoding(this->_header);

	oldPos = 0;
	while ((pos = this->_header.find(CRLF, oldPos)) != std::string::npos)
	{
		if (!pos)
			return;
		line = this->_header.substr(oldPos, pos - oldPos);
		colPos = line.find(":");
		if (colPos != std::string::npos)
		{
			key = line.substr(0, colPos);
			value = Util::lrtrim(line.substr(colPos + 1));
			this->_headers.insert(std::make_pair(key, value));
		}
		oldPos = pos + 2;
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
	if (this->_transferEncoding == "chunked")
	{
		this->_body = extractChunkedBody(this->_body);
		// body size가 아니라 모든 chunked 값 더하기?
		this->_contentLength = static_cast<unsigned int>(this->_body.size());
	}
}

/**
 * @brief isAllSet
 *
 * HTTP Request에 필수적인 헤더들이 모두 설정되었는지 확인합니다.
 *
 * @param void
 * @return bool
 */
bool	Request::isAllSet(void) const
{
	return !this->_header.empty() && !this->_method.empty()
		&& !this->_requestUrl.empty() && !this->_httpVersion.empty();
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
	this->_body.clear();
	this->_contentLength = 0;
	this->_transferEncoding.clear();
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
	std::size_t	readCount = 1;

	while ((pos = body.find(CRLF, oldPos)) != std::string::npos)
	{
		line = body.substr(oldPos, pos - oldPos);
		if (readCount & 1)
		{
			chunkSize = readChunkSize(line);
			if (chunkSize == 0)
				break;
		}
		else
			chunkedBody.append(line);
		oldPos = pos + 2;
		readCount++;
	}
	return chunkedBody;
}
