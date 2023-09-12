/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:09 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/11 23:52:44 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#define CRLF "\r\n"
#define DOUBLE_CRLF "\r\n\r\n"

#include <cstdlib>
#include <string>
#include <map>
#include "Util.hpp"

class	Request
{
	private:
		std::string							_rawData;
		std::string							_header;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		unsigned int						_contentLength;
		std::string							_transferEncoding;
		std::string							_method;
		std::string							_requestUrl;
		std::string							_httpVersion;

	public:
		Request(void);
		Request(std::string const& data);
		Request(Request const& target);
		~Request(void);
		Request&						operator=(Request const& target);

	public:
		std::string const&				getRawData(void) const;
		void							setRawData(std::string const rawData);
		std::string const&				getHttpHeader(void) const;
		std::map<std::string, std::string> const&
										getHttpHeaders(void) const;
		std::string const&				getHttpBody(void) const;
		unsigned int					getContentLength(void) const;
		std::string const&				getTransferEncoding(void) const;
		std::string const&				getHttpMethod(void) const;
		std::string const&				getRequestUrl(void) const;
		std::string const&				getHttpVersion(void) const;

	public:
		void							updateRequest(void);
		void							updateRequestLine(void);
		void							updateHttpHeader(void);
		void							updateHttpBody(void);
		bool							isAllSet(void) const;
		void							resetRequest(void);
};

#endif	/* __REQUEST_HPP__ */
