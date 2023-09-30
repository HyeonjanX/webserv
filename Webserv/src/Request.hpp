/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:09 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/19 17:44:14 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#define CRLF "\r\n"
#define DOUBLE_CRLF "\r\n\r\n"

#include <iostream> // for debug
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include "Util.hpp"

struct Header
{
	std::string	key;
	std::string	value;

	Header() : key(""), value("") {}
    Header(const std::string& k, const std::string& v) : key(k), value(v) {}
};

struct Content
{
	std::string	name;
	std::string	filename;
	std::string	type;
	std::string	data;
};

class	Request
{
	private:
		std::string				_rawData;
		std::vector<Header>		_headers;
		std::string				_body;
		std::string				_chunkOctetData;
		unsigned int			_contentLength;
		std::string				_transferEncoding;
		std::string				_contentType;
		std::string				_expected;
		std::vector<Content>	_contents;
		std::string				_method;
		std::string				_requestUrl;
		std::string				_httpVersion;

	public:
		Request(void);
		Request(Request const& target);
		~Request(void);
		Request&					operator=(Request const& target);

	public:
		std::string const&			getRawData(void) const;
		void						setRawData(std::string const rawData);
		std::vector<Header> const&	getHttpHeaders(void) const;
		std::string const&			getHttpBody(void) const;
		unsigned int				getContentLength(void) const;
		std::string const&			getTransferEncoding(void) const;
		std::string const&			getContentType(void) const;
		std::vector<Content> const&	getContents(void) const;
		std::string const&			getHttpMethod(void) const;
		std::string const&			getRequestUrl(void) const;
		std::string					getRequestPath(void) const;
		std::string const&			getHttpVersion(void) const;
		std::string const&			getChunkOctetData(void) const;
		std::string&				getChunkOctetData(void);

	public:
		void						parseRequestLine(const std::string &requestLine);
		void						updateHeaderValue(
										std::string const& key,
										std::string const& valuei
									);
		std::string					findHeaderValue(std::string const& key);
		void						handleMultipartBody(void);
		void						resetRequest(void);

		// data
		void						appendRawData(const std::vector<char> &buffer, ssize_t bytes_read);
		void						appendHeader(const std::string &key, const std::string &val);

	public:
		int							handleHeaders(std::string &hostname);
		bool						extractContentTypeData(std::string fieldValue, std::string &mediType, std::map<std::string, std::string> &parameters);
		std::string					extractBoundary(std::string fieldValue);
		std::vector<Content>		extractMultipartBody(std::string const &body, std::string &boundary);
};

#endif	/* __REQUEST_HPP__ */
