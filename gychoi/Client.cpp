/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/06 19:52:55 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/06 20:52:28 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

static void	throwError(std::string const& msg);

/**
 * Constructor & Destroctor
 */

Client::Client(void) : _sock(0), _port(0), _addrlen(sizeof(this->_addr))
{
	memset(&(this->_addr), 0x0, sizeof(this->_addr));
	memset(this->_buffer, 0x0, sizeof(this->_buffer));
}

Client::Client(int serverSocket) : _addrlen(sizeof(this->_addr))
{
	if ((this->_sock = accept(serverSocket,
			reinterpret_cast<struct sockaddr*>(&(this->_addr)),
			&(this->_addrlen))) == -1)
		throwError("Client Error: accept");
	if (fcntl(this->_sock, F_SETFL, O_NONBLOCK) == -1)
	{
		close(this->_sock);
		throwError("Client Error: fcntl");
	}
	this->_port = ntohs(this->_addr.sin_port);
	std::cout << "[INFO] : Client connected: " << this->_port << std::endl;
}

Client::Client(Client const& target)
{
	*this = target;
}

Client&	Client::operator=(Client const& target)
{
	if (this != &target)
	{
		this->_sock = target.getClientSocket();
		this->_port = target.getClientPort();
		this->_addr = target.getClientAddr();
		this->_addrlen = target.getClientAddrlen();
		std::copy(target.getClientBuffer(), 
			target.getClientBuffer() + BUFFER_SIZE, this->_buffer);
	}
	return *this;
}

Client::~Client(void) {}

/**
 * Getter & Setter
 */

int	Client::getClientSocket(void) const
{
	return this->_sock;
}

int	Client::getClientPort(void) const
{
	return this->_port;
}

struct sockaddr_in const&	Client::getClientAddr(void) const
{
	return this->_addr;
}

socklen_t	Client::getClientAddrlen(void) const
{
	return this->_addrlen;
}

char const*	Client::getClientBuffer(void) const
{
	return this->_buffer;
}

void	Client::setClientBuffer(char* buffer)
{
	std::copy(buffer, buffer + BUFFER_SIZE, this->_buffer);
}

/**
 * Member Function
 */

/**
 * Initialize Function
 */

/**
 * Utility Function
 */

static void	throwError(std::string const& msg)
{
	std::string	errorMessage = msg + ": " + strerror(errno);

	throw std::runtime_error(errorMessage);
}
