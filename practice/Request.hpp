/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:09 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/05 22:27:34 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#define CRLF "\r\n"
#define DOUBLE_CRLF "\r\n\r\n"

#include <string>
#include <cstdlib>

class	Request
{
	private:
		std::string const	_rawData;
		std::string	const	_header;
		std::string	const	_body;
		unsigned int		_contentLength;
		std::string const	_method;
		std::string const	_requestUrl;
		std::string const	_httpVersion;

	public:
		Request(void);
		Request(std::string const& data);
		Request(Request const& target);
		Request& operator=(Request const& target);
		~Request(void);

	public:
		std::string const&	getRawData(void) const;
		void				setRawData(std::string const& rawData) const;
		std::string const&	getHttpHeader(void) const;
		std::string const&	getHttpBody(void) const;
		unsigned int		getContentLength(void) const;
		std::string const&	getHttpMethod(void) const;
		std::string const&	getRequestUrl(void) const;
		std::string const&	getHttpVersion(void) const;

	public:
		void	updateRequest(void) const;
		bool	isAllSet(void) const;
};

#endif	/* __REQUEST_HPP__ */
