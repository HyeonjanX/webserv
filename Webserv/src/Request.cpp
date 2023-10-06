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
// url에서 ?를 경계로 path만 추출하여 전달
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

	if (DEBUG)
	{
		std::cout << GREEN << "|" << _method << "| |" << _requestUrl << "| |" << _httpVersion << "|" << RESET << std::endl;
	}

	// _method 유효성 검사 X => token이여서 제한이 쫌 없다.

	// _requestUrl 유효성검사
	if (!Util::startsWith(_requestUrl, "/") ||
		_requestUrl.find("../") != std::string::npos ||
		_requestUrl.find("./") != std::string::npos ||
		Util::endsWith(_requestUrl, "/.") ||
		Util::endsWith(_requestUrl, "/.."))
	{
		throw 400; // Bad Request
	}

	// _httpVersion 유효성검사
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

// "Content-Disposition" ":" "form-data" *(";" disposition-parm) CRLF
// ["Content-Type" ":" MimeType CRLF]
// [CRLF *OCTET]
static void	parseBodyPart(std::vector<Content> &contents, const std::string &originBody, const std::size_t startPos, const std::size_t endPos)
{
	std::string body = originBody.substr(startPos, endPos - startPos);
	std::cout << CYAN << "parseBodyPart애서 body체크|" << body << "|끝" << RESET << std::endl;
	Content content;
	// const char *LWSP_CHARS = " \t";
	const char *CRLF_CHARS = "\r\n";

	std::map<std::string, std::string> dispositionParams;

	// 1. "Content-Disposition" ":" "form-data" *(";" disposition-parm) CRLF

	std::string::size_type crlfPos = body.find(CRLF_CHARS);

	if (crlfPos == std::string::npos)
	{
		throw "Content-Disposition 헤더의 끝을 의미할 CRLF가 없습니다.";
	}
	std::cout << "crlfPos :" << crlfPos << std::endl;
	std::string contentDispositionHeader = body.substr(0, crlfPos);

	std::string::size_type colonPos = contentDispositionHeader.find(':');

	if (colonPos == std::string::npos)
	{
		std::cout << "|" << contentDispositionHeader << "|" << std::endl;
		throw "Content-Disposition 헤더의 key : value를 나눌 콜론이 없습니다.";
	}

	std::string contentDispositionHeaderKey = contentDispositionHeader.substr(0, colonPos);
	std::string contentDispositionHeaderValue = Util::ltrim(contentDispositionHeader.substr(colonPos + 1));

	if (!Util::caseInsensitiveCompare(contentDispositionHeaderKey, "Content-Disposition") ||
		!Util::startsWith(contentDispositionHeaderValue, "form-data"))
	{
		std::cout << "|" << contentDispositionHeaderKey << "|" << contentDispositionHeaderValue << "|" << std::endl;
		throw "Content-Disposition: form-data를 만족하지 않습니다.";
	}

	std::string dispositionParms = contentDispositionHeaderValue.substr(std::string("form-data").size());

	std::string::size_type pos = 0, oldPos = 0;

	while (oldPos < dispositionParms.size())
	{
		if (dispositionParms[oldPos] != ';')
		{
			throw "disposition-parm가 세미콜론으로 시작하지 않았습니다.";
		}

		std::string::size_type crlfPos2 = dispositionParms.find(';', oldPos + 1);
		if (crlfPos2 == std::string::npos)
		{
			crlfPos2 = dispositionParms.size();
		}
		std::string keyValue = Util::lrtrim(dispositionParms.substr(oldPos + 1, crlfPos2 - (oldPos + 1)));

		std::string::size_type equalsPos = keyValue.find('=');

		if (equalsPos == std::string::npos)
		{
			throw "disposition-parm가 key=value가 아닙니다.";
		}

		std::string key = keyValue.substr(0, equalsPos);
		std::string value = Util::lrdtrim(keyValue.substr(equalsPos + 1), "\"");

		std::map<std::string, std::string>::iterator it = dispositionParams.find(key);

		if (it != dispositionParams.end())
		{
			throw "disposition-parm의 key값이 중복되었습니다.";
		}
		std::cout << YELLOW << "값추가 => key: " << key << ", value: " << value << RESET << std::endl;
		dispositionParams[key] = value;

		oldPos = crlfPos2;
	}

	std::map<std::string, std::string>::iterator nameIt = dispositionParams.find("name");
	if (nameIt == dispositionParams.end())
	{
		throw "disposition-parm에 name이 없습니다";
	}
	// nameIt->second 활용하기
	content.name = nameIt->second;

	std::map<std::string, std::string>::iterator filenameIt = dispositionParams.find("filename");
	if (filenameIt != dispositionParams.end())
	{
		// filenameIt->second 활용하기
		content.filename = filenameIt->second;
	}

	// "Content-Disposition" ":" "form-data" *(";" disposition-parm) CRLF 에서 다음줄로
	oldPos = crlfPos + 2;
	if (oldPos == body.size())
	{
		// 필수인 Content-Disposition 헤더만 있었음
		contents.push_back(content);
		return;
	}

	if ((pos = body.find(CRLF_CHARS, oldPos)) == std::string::npos)
	{
		throw "Content-Disposition 헤더 이후 부분이 잘 못 되었습니다.(CRLF가 없음)";
	}

	// [CRLF *OCTET]
	if (pos == oldPos)
	{
		content.data = body.substr(oldPos + 2);
		return;
	}

	// ["Content-Type" ":" MimeType CRLF]
	std::string contentTypeHeader = body.substr(oldPos, pos - oldPos);
	std::cout << "contentTypeHeader: |" << contentTypeHeader << "|" << std::endl;

	std::string::size_type colonPos2 = contentTypeHeader.find(':');

	if (colonPos2 == std::string::npos)
	{
		throw "Content-Type 헤더의 key : value를 나눌 콜론이 없습니다.";
	}

	std::string contentTypeHeaderKey = contentTypeHeader.substr(0, colonPos2);
	std::string contentTypeHeaderValue = Util::lrtrim(contentTypeHeader.substr(colonPos2 + 1));

	if (!Util::caseInsensitiveCompare(contentTypeHeaderKey, "Content-Type"))
	{
		std::cout << RED << "|" << contentTypeHeaderKey << "|" << std::endl;
		std::cout << RED << "|" << contentTypeHeaderValue << "|" << std::endl;
		throw "Content-Disposition 헤더 이후에는 Content-Type 헤더만 나올 수 있습니다.";
	}
	
	// contentTypeHeaderValue 활용하기
	content.type = contentTypeHeaderValue;

	oldPos = pos + 2;
	
	// [CRLF *OCTET]
	if (oldPos != body.size())
	{
		if (body.substr(oldPos, 2).compare(CRLF_CHARS) != 0)
		{
			throw "데이터는 CRLF 구분 후, 시작되어야 합니다.";
		}
		content.data = body.substr(oldPos + 2);
	}

	contents.push_back(content);

}

// [preamble CRLF]
// dash-boundary *LWSP-char CRLF
// body-part
// *(delimiter *LWSP-char CRLF body-part)
// close-delimiter *LWSP-char
// [CRLF epilogue]
std::vector<Content> Request::extractMultipartBody(std::string const &body, std::string const &boundary)
{
	const std::string dashBoundary = std::string("--") + boundary;
	const std::string delimiter = std::string("\r\n") + dashBoundary;
	const char *LWSP_CHARS = " \t";
	const char *CRLF_CHARS = "\r\n";
	std::vector<Content> contents;
	std::size_t pos = 0, oldPos = 0;

	try
	{
		// 1. [preamble CRLF]
		if ((pos = body.find(dashBoundary)) == std::string::npos)
		{
			throw "초기 dash-boundary가 없습니다.";
		}

		std::string preamble = body.substr(0, pos);
		if (!preamble.empty() && !Util::endsWith(preamble, CRLF_CHARS))
		{
			throw "preamble뒤에 CRLF가 따라나오지 않았습니다.";
		}

		// 2. <oldPos> dash-boundary *LWSP-char <pos> CRLF
		oldPos = pos;
		if ((pos = body.find(CRLF_CHARS, oldPos + dashBoundary.size())) == std::string::npos)
		{
			throw "초기 dash-boundary 뒤에 따라올 CRLF가 없습니다.";
		}
		// *LWSP-char
		if (!Util::isAllLWSP(body, oldPos + dashBoundary.size(), pos))
		{
			throw "초기 dash-boundary 뒤에 따라나오는 LWSP-char가 잘 못 되었습니다.";
		}

		oldPos = pos + 2;

		// body-part
		// *(delimiter *LWSP-char CRLF body-part)
		// close-delimiter *LWSP-char
		while (42)
		{
			// body-part
			if ((pos = body.find(delimiter.c_str(), oldPos)) == std::string::npos)
			{
				throw "body-part 끝을 알려줄 delimiter가 없습니다.";
			}
			if (body.substr(pos + delimiter.size(), 2).compare("--") == 0)
			{
				// close-delimiter *LWSP-char
				std::string bodyPart = body.substr(oldPos, pos - oldPos);
				std::cout << MAGENTA << "|" << bodyPart << "|" << RESET << std::endl;
				parseBodyPart(contents, body, oldPos, pos);
				break;
			}
			// else if (pos - oldPos < 2 || body.substr(pos - 2, 2).compare(CRLF_CHARS) != 0)
			// {
			// 	throw "dash-boundary 앞에 CRLF가 나오지 않았습니다."; // error
			// }
			else
			{
				// *(delimiter *LWSP-char CRLF body-part)
				std::string bodyPart = body.substr(oldPos, pos - oldPos);
				std::cout << GREEN << "|" << bodyPart << "|" << RESET << std::endl;
				parseBodyPart(contents, body, oldPos, pos);
				oldPos = pos + delimiter.size(); // <oldPos> *LWSP-char CRLF body-part
				if ((pos = body.find(CRLF_CHARS, oldPos)) == std::string::npos)
				{
					throw "body-part를 구분 짓는 delimiter 뒤에 CRLF가 따라 오지 않았습니다.";
				}

				if (!Util::isAllLWSP(body, oldPos, pos))
				{
					throw "body-part를 구분 짓는 delimiter 뒤에 *LWSP-char가 올바르지 않습니다.";
				}
				oldPos = pos + 2;
			}
		}
		// <pos>close-delimiter
		// *LWSP-char [CRLF epilogue]
		oldPos = pos + delimiter.size() + 2;
		pos = body.find_first_not_of(LWSP_CHARS, oldPos);
		if (pos != std::string::npos && body.substr(pos, 2).compare(CRLF_CHARS) != 0)
		{
			std::cout << GREEN << "|" << body.substr(oldPos) << "|" << RESET << std::endl;
			std::cout << GREEN << "|" << body.substr(pos) << "|" << RESET << std::endl;
			throw "epilogue가 잘 못 되었습니다.";
		}
	}
	catch (const char *msg)
	{
		std::cerr << msg << std::endl;
		throw 400;
	}

	return contents;
}

void	Request::handleHeaders(std::string &hostname, bool &expected100)
{
	const bool DEBUG = false;

	for (std::vector<Header>::const_iterator it = _headers.begin(); it < _headers.end(); ++it)
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
			expected100 = true;
		}

		if (it->key == "host")
		{
			hostname = it->value;
		}
	}
}

// RFC 7230
// header-field   = field-name ":" OWS field-value OWS
// field-name     = token
// field-value    = *( field-content / obs-fold )
// field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]

// RFC7231#appendix-D
// Content-Type = media-type

// RFC7231#section-3.1.1.1
// media-type = type "/" subtype *( OWS ";" OWS parameter )
// parameter = token "=" ( token / quoted-string )
//  type       = token
//  subtype    = token

bool Request::extractContentTypeData(std::string fieldValue, std::string &mediType, std::map<std::string, std::string> &parameters)
{
	const char *OWS_SEMI_COLON = " \t;";

	std::string::size_type pos, oldPos;

	fieldValue = Util::lrtrim(fieldValue);

	pos = fieldValue.find_first_of(OWS_SEMI_COLON);

	// type "/" subtype
	if (pos == std::string::npos)
	{
		mediType = fieldValue;
		return true;
	}
	mediType = fieldValue.substr(0, pos);

	std::cout << BLUE << "extractContentTypeData" << std::endl;

	while (pos != std::string::npos)
	{
		std::cout << BLUE << "extractContentTypeData inner 1" << std::endl;
		std::string parameter, key, value;

		// *( OWS ";" OWS parameter ) 에서 parameter 시작점
		oldPos = fieldValue.find_first_not_of(OWS_SEMI_COLON, pos);

		std::cout << BLUE << "extractContentTypeData inner 2" << std::endl;
		if (oldPos == std::string::npos)
		{
			return false;
		}

		pos = fieldValue.find_first_of(OWS_SEMI_COLON, oldPos); // parameter 끝점

		std::cout << BLUE << "extractContentTypeData inner 3" << std::endl;
		if (pos == std::string::npos)
		{
			parameter = fieldValue.substr(oldPos);
			std::cout << BLUE << "extractContentTypeData inner 4" << std::endl;
		}
		else
		{
			parameter = fieldValue.substr(oldPos, pos - oldPos);
		}

		std::size_t equalPos = parameter.find('=');

		if (equalPos == std::string::npos)
		{
			return false;
		}

		key = parameter.substr(0, equalPos);
		value = parameter.substr(equalPos + 1);

		std::cout << BLUE << "extractContentTypeData inner 5" << std::endl;

		// 빈 값 && 중복 불허
		if (key.empty() || value.empty() || parameters.find(key) != parameters.end())
		{
			return false;
		}

		parameters[key] = value;
		// oldPos = pos;
	}

	return true;
}

std::string Request::extractBoundary(std::string fieldValue)
{
	std::string mediType, boundary;
	std::map<std::string, std::string> parameters;

	// 유효한 Content-Type
	if (extractContentTypeData(fieldValue, mediType, parameters))
	{
		// 멀티파트/폼데이터 => 바운더리 추출
		if (mediType.compare("multipart/form-data") == 0)
		{
			std::map<std::string, std::string>::iterator it = parameters.find("boundary");
			if (it != parameters.end() && Util::isValidBoundary(it->second))
			{
				boundary = it->second; // boundary;
			}
		}
	}
	return boundary;
}

// const std::string &Request::getPostData()
std::string Request::getPostData()
{
	static const std::string &emptyString("");
	const std::string &boundary = extractBoundary(findHeaderValue("content-type"));

	if (boundary.empty())
	{
		// 1. chunked || 2. normal
		// std::cout << CYAN << "Encoding: " << getTransferEncoding() << RESET << std::endl;
		return getTransferEncoding().compare("chunked") == 0 ? getChunkOctetData() : getRawData();
	}
	// std::cout << CYAN << "boundary: " << boundary << RESET << std::endl;
	
	// 3. mutipart/form-data, throw 400
	const std::vector<Content> &contents = extractMultipartBody(getTransferEncoding().compare("chunked") == 0 ? getChunkOctetData() : getRawData(), boundary);
	
	for (std::vector<Content>::const_iterator it = contents.begin(); it < contents.end(); ++it)
	{
		// std::cout << "============================================" << std::endl;
		// std::cout << RED << "name: |" << it->name << "|" << RESET << std::endl;
		// std::cout << BLUE << "filename: |" << it->filename << "|" << RESET << std::endl;
		// std::cout << RED << "type: |" << it->type << "|" << RESET << std::endl;
		// std::cout << BLUE << "data: |" << it->data << "|" << RESET << std::endl;
		std::cout << "============================================" << std::endl;

		if (it->name.compare("file") == 0)
		{
			// std::cout << "return it->data: " << it->data.size() << ", " << it->data << std::endl;
			return it->data;
		}
	}
	// std::cout << "return emptyString" << std::endl;
	return emptyString;
}