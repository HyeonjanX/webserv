/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:09 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/15 00:00:58 by gychoi           ###   ########.fr       */
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
		std::string				_header; // need to remove
		std::vector<Header>		_headers;
		std::string				_body;
		unsigned int			_contentLength;
		std::string				_transferEncoding;
		std::string				_contentType;
		std::vector<Content>	_contents;
		std::string				_method;
		std::string				_requestUrl;
		std::string				_httpVersion;

	public:
		Request(void);
		Request(std::string const& data);
		Request(Request const& target);
		~Request(void);
		Request&						operator=(Request const& target);

	public:
		std::string const&			getRawData(void) const;
		void						setRawData(std::string const rawData);
		std::string const&			getHttpHeader(void) const;
		std::vector<Header> const&	getHttpHeaders(void) const;
		std::string const&			getHttpBody(void) const;
		unsigned int				getContentLength(void) const;
		std::string const&			getTransferEncoding(void) const;
		std::string const&			getContentType(void) const;
		std::vector<Content> const&	getContents(void) const;
		std::string const&			getHttpMethod(void) const;
		std::string const&			getRequestUrl(void) const;
		std::string const&			getHttpVersion(void) const;

	public:
		void						updateRequestLine(void);
		void						updateHttpHeader(void);
		void						updateHeaderValue(std::string const& key,
									std::string const& value);
		void						updateHttpBody(void);
		void						handleChunkedBody(void);
		void						handleMultipartBody(void);
		bool						isLastChunk(void) const;
		void						resetRequest(void);
};

#endif	/* __REQUEST_HPP__ */
