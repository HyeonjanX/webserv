/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/05 16:33:09 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/06 15:52:32 by gychoi           ###   ########.fr       */
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
		std::string		_rawData;
		std::string		_header;
		std::string		_body;
		unsigned int	_contentLength;
		std::string		_method;
		std::string		_requestUrl;
		std::string		_httpVersion;

	public:
		Request(void);
		Request(std::string const& data);
		Request(Request const& target);
		Request& operator=(Request const& target);
		~Request(void);

	public:
		std::string const&	getRawData(void) const;
		void				setRawData(std::string const& rawData);
		std::string const&	getHttpHeader(void) const;
		std::string const&	getHttpBody(void) const;
		unsigned int		getContentLength(void) const;
		std::string const&	getHttpMethod(void) const;
		std::string const&	getRequestUrl(void) const;
		std::string const&	getHttpVersion(void) const;

	public:
		void	updateRequest(void);
		bool	isAllSet(void) const;
};

#endif	/* __REQUEST_HPP__ */
