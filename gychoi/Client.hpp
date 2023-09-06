/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/06 19:48:45 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/06 23:11:02 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#define BUFFER_SIZE 1024

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Request.hpp"

class	Client : public Request // 상속 제거해야 함.
{
	private:
		int					_sock;
		int					_port;
		struct sockaddr_in	_addr;
		socklen_t			_addrlen;
		char				_buffer[BUFFER_SIZE];
		Request				_req;

	public:
		Client(void);
		Client(int serverSocket);
		Client(Client const& target);
		Client& operator=(Client const& target);
		~Client(void);

	public:
		int							getClientSocket(void) const;
		int							getClientPort(void) const;
		struct sockaddr_in const&	getClientAddr(void) const;
		socklen_t					getClientAddrlen(void) const;
		char const*					getClientBuffer(void) const;
		void						setClientBuffer(char* buffer);
};

#endif	/* __CLIENT_HPP__ */
