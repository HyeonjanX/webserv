/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:39 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/06 23:08:47 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

static std::string	extractHttpHeader(std::string const& rawData);
static std::string	extractHttpBody(std::string const& rawData);
static unsigned int	extractContentLength(std::string const& header);
static std::string	extractHttpMethod(std::string const& header);
static std::string	extractRequestUrl(std::string const& header);
static std::string	extractHttpVersion(std::string const& header);

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
	this->_method = extractHttpMethod(this->_header);
	this->_requestUrl = extractRequestUrl(this->_header);
	this->_httpVersion = extractHttpVersion(this->_header);
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

std::string const&	Request::getHttpBody(void) const
{
	return this->_body;
}

unsigned int	Request::getContentLength(void) const
{
	return this->_contentLength;
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
 * _rawData에 저장된 HTTP 문자열을 바탕으로 Request 멤버 변수를 업데이트 합니다.
 *
 * @param void
 * @return void
 */
void	Request::updateRequest(void)
{
	this->_header = extractHttpHeader(this->_rawData);
	this->_body = extractHttpBody(this->_rawData);
	this->_contentLength = extractContentLength(this->_header);
	this->_method = extractHttpMethod(this->_header);
	this->_requestUrl = extractRequestUrl(this->_header);
	this->_httpVersion = extractHttpVersion(this->_header);
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
	return !this->_rawData.empty() && !this->_header.empty() &&
		!this->_method.empty() && !this->_requestUrl.empty() &&
		!this->_httpVersion.empty();
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
	this->_method.clear();
	this->_requestUrl.clear();
	this->_httpVersion.clear();
}

/**
 * Initialize Function
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
