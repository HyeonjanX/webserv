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

static std::vector<Content> extractMultipartBody(std::string const &body, std::string &boundary);

/**
 * Constructor & Destroctor
 */

Request::Request(void) : _contentLength(0) {}

Request::Request(Request const &target)
{
	*this = target;
}

// 현재 얕은 복사로 되어있음
Request &Request::operator=(Request const &target)
{
	if (this != &target)
	{
		this->_rawData = target.getRawData();
		this->_headers = target.getHttpHeaders();
		this->_body = target.getHttpBody();
		this->_chunkOctetData = target._chunkOctetData;
		this->_contentLength = target.getContentLength();
		this->_transferEncoding = target.getTransferEncoding();
		this->_contentType = target.getContentType();
		this->_expected = target._expected;
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

std::string const &Request::getRawData(void) const
{
	return this->_rawData;
}

void Request::setRawData(std::string const rawData)
{
	this->_rawData = rawData;
}

std::vector<Header> const &Request::getHttpHeaders(void) const
{
	return this->_headers;
}

std::string const &Request::getHttpBody(void) const
{
	return this->_body;
}

unsigned int Request::getContentLength(void) const
{
	return this->_contentLength;
}

std::string const &Request::getTransferEncoding(void) const
{
	return this->_transferEncoding;
}

std::string const &Request::getContentType(void) const
{
	return this->_contentType;
}

std::vector<Content> const &Request::getContents(void) const
{
	return this->_contents;
}

std::string const &Request::getHttpMethod(void) const
{
	return this->_method;
}

std::string const &Request::getRequestUrl(void) const
{
	return this->_requestUrl;
}
std::string	Request::getRequestPath(void) const
{
	std::size_t pos = _requestUrl.find('?');
	return pos ? _requestUrl.substr(0, pos) : _requestUrl;
}

std::string const &Request::getHttpVersion(void) const
{
	return this->_httpVersion;
}

std::string const &Request::getChunkOctetData(void) const
{
	return _chunkOctetData;
}

std::string &Request::getChunkOctetData(void)
{
	return _chunkOctetData;
}

void Request::parseRequestLine(std::string const &requestLine)
{
	const bool DEBUG = true;
	const char sp = ' ';

	std::size_t pos1;
	std::size_t pos2;

	if (DEBUG)
	{
		std::cout << GREEN << "요청라인: " << requestLine << RESET << std::endl;
	}

	if ((pos1 = requestLine.find(sp)) == std::string::npos ||
		(pos2 = requestLine.find(sp, pos1 + 1)) == std::string::npos)
		throw 400; // Bad Request

	_method = requestLine.substr(0, pos1);
	_requestUrl = Util::urlDecode(requestLine.substr(pos1 + 1, pos2 - (pos1 + 1)));
	_httpVersion = requestLine.substr(pos2 + 1);

	// TODO: 각각 유효성 검사

	if (DEBUG)
	{
		std::cout << GREEN << "|" << _method << "| |" << _requestUrl << "| |" << _httpVersion << "|" << RESET << std::endl;
	}

	if (_httpVersion.compare(std::string("HTTP/1.1")) != 0)
	{
		if (DEBUG)
		{
			std::cout << GREEN << 505 << RESET << std::endl;
		}
		throw 505; // HTTP Version Not Supported
	}
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
void Request::updateHeaderValue(std::string const &key, std::string const &newValue)
{
	std::vector<Header> &headers = this->_headers;

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
std::string Request::findHeaderValue(std::string const &key)
{
	std::vector<Header> &headers = this->_headers;

	for (std::vector<Header>::iterator hit = headers.begin();
		 hit != headers.end(); ++hit)
	{
		if (hit->key == Util::toLowerCase(key))
			return hit->value;
	}
	return std::string();
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
void Request::handleMultipartBody(void)
{
	std::string boundary;
	std::size_t pos;

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
 * @brief resetRequest
 *
 * Reqeust를 초기화합니다.
 *
 * @param void
 * @return void
 */
void Request::resetRequest(void)
{
	this->_rawData.clear();
	this->_headers.clear();
	this->_body.clear();
	this->_chunkOctetData.clear();
	this->_contentLength = 0;
	this->_transferEncoding.clear();
	this->_contentType.clear();
	this->_expected.clear();
	this->_contents.clear();
	this->_method.clear();
	this->_requestUrl.clear();
	this->_httpVersion.clear();
}

/**
 * @brief appendRawData
 *
 * rawData에 방금 읽어들인 recvData를 추가합니다.
 *
 * @param void
 * @return void
 */
void Request::appendRawData(const std::vector<char> &buffer, ssize_t bytes_read)
{
	this->_rawData.append(buffer.data(), bytes_read);
}

void Request::appendHeader(const std::string &key, const std::string &val)
{
	_headers.push_back(Header(key, val));
}

/**
 * Helper Function
 */

static std::vector<Content> extractMultipartBody(std::string const &body, std::string &boundary)
{
	std::vector<Content> contents;
	std::vector<std::string> tokens;
	std::vector<std::string> pair;
	std::string line;
	std::size_t pos;
	std::size_t oldPos = 0;
	bool flag = true;

	// boundary validation check
	boundary = "--" + boundary;
	while (((pos = body.find(boundary, oldPos)) != std::string::npos) && flag == true)
	{
		Content entry;
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

int	Request::handleHeaders(std::string &hostname)
{
	std::vector<Header>::const_iterator it = _headers.begin();
	const bool DEBUG = false;
	int statusCode = 0;

	for (; it < _headers.end(); ++it)
	{
		if (DEBUG)
		{
			std::cout << "handleHeaders >> " << CYAN << it->key << ": " << it->value << RESET << std::endl;
		}

		if (it->key == "transfer-encoding" && it->value == "chunked")
		{
			_transferEncoding = "chunked";
		}

		if (it->key == "content-length")
		{
			_contentLength = static_cast<size_t>(Util::ft_atol(it->value.c_str(), 10));
			// std::cout << "content-length => " << it->value;
			// _contentLength = static_cast<size_t>(Util::ft_atol(it->value, 10));
		}

		if (it->key == "expect" && it->value == "100-continue")
		{
			statusCode = 100;
		}

		if (it->key == "host")
		{
			hostname = it->value;
		}
	}

	return statusCode;
}