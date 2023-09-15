/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/06 19:52:55 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/15 23:03:35 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

static void	checkRequestLine(Client& client);
static void	checkHttpHeader(Client& client);
static void	checkHttpBody(Client& client);
static void	validateReadStatus(Client& client);
static void	purifyHttpBody(Client& client);
static void	throwError(std::string const& msg);

/**
 * Constructor & Destroctor
 */

Client::Client(void) : _sock(0), _port(0), _addrlen(sizeof(this->_addr)),
						_status(BEFORE_READ)
{
	memset(&(this->_addr), 0x0, sizeof(this->_addr));
	memset(this->_buffer, 0x0, sizeof(this->_buffer));
}

Client::Client(int serverSocket) : _addrlen(sizeof(this->_addr)),
									_status(BEFORE_READ)
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
	memset(this->_buffer, 0x0, sizeof(this->_buffer));
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
		this->_request = target.getClientRequest();
		this->_status = target.getClientStatus();
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

Request const&	Client::getClientRequest(void) const
{
	return this->_request;
}

Request&	Client::getClientRequest(void)
{
	return this->_request;
}

void	Client::setClientRequest(Request request)
{
	this->_request = request;
}

short	Client::getClientStatus(void) const
{
	return this->_status;
}

void	Client::setClientStatus(short status)
{
	this->_status = status;
}

/**
 * Member Function
 */

/**
 * @brief readRequest
 *
 * Client 객체가 recv로 받아온 정보를 바탕으로,
 * _request 멤버 구조체 내 정보를 업데이트하고, Client의 _status를 갱신합니다.
 *
 * @param void
 * @return bool
 */
bool	Client::readClientRequest(void)
{
	ssize_t	readByte;

	if ((readByte = recv(this->_sock, const_cast<char*>(this->_buffer),
					sizeof(this->_buffer), 0)) == -1)
	{
		throwError("recv failed"); // need to change
		// status = 500?
	}
	else if (readByte == 0)
		return true;
	else
	{
		std::string	s = this->_request.getRawData();
		s.append(this->_buffer, static_cast<std::size_t>(readByte));
		this->_request.setRawData(s);
		if (this->_status == BEFORE_READ)
			checkRequestLine(*this);
		if (this->_status == READ_REQUESTLINE)
			checkHttpHeader(*this);
		if (this->_status == READ_HEADER || this->_status == READING)
			checkHttpBody(*this);
		if (this->_status == READ_BODY)
			validateReadStatus(*this);
		if (this->_status == READ_END)
			purifyHttpBody(*this);
	}
	return true;
}

bool	Client::uploadClientFile(void)
{
	Request					req = this->_request;
	std::vector<Content>	contents = req.getContents();

	std::cout << contents.size() << std::endl;
	if (contents.empty())
		return true;

	for (std::vector<Content>::iterator cit = contents.begin();
		cit != contents.end(); ++cit)
	{
		if (!cit->filename.empty())
		{
			std::ofstream	uploadFile(cit->filename.c_str(), std::ios::binary);

			if (uploadFile.is_open())
			{
				uploadFile.write(cit->data.c_str(), static_cast<std::streamsize>
								(cit->data.size()));
				uploadFile.close();
			}
			else
			{
				std::cout << "ERROR: Cannot create " << cit->filename << std::endl;
				return false;
			}
		}
	}
	return true;
}

//bool	Client::writeRequest(void)
//{
//	// Response에서 구현해야 하는 부분.
//	// status code에 따라 다른 페이지 반환.
//}

/**
 * Helper Function
 */

static void	checkRequestLine(Client& client)
{
	Request&	request = client.getClientRequest();

	request.updateRequestLine();
	if (request.getHttpMethod().empty() || request.getRequestUrl().empty()
		|| request.getHttpVersion().empty())
		return;
	client.setClientStatus(READ_REQUESTLINE);
}

static void	checkHttpHeader(Client& client)
{
	Request&	request = client.getClientRequest();

	request.updateHttpHeader();
	if (request.getHttpHeader().empty())
		return;
	client.setClientStatus(READ_HEADER);
}

static void	checkHttpBody(Client& client)
{
	Request&	request = client.getClientRequest();

	request.updateHttpBody();
	client.setClientStatus(READ_BODY);
}

static void	validateReadStatus(Client& client)
{
	Request const&		request = client.getClientRequest();
	std::string const&	httpBody = request.getHttpBody();
	unsigned int		contentLength = request.getContentLength();

	// 계속되는 Reading 상태로 머물 수 있음.
	// Timeout 세팅 필요.
	if (request.getTransferEncoding() == "chunked")
	{
		if (request.isLastChunk())
			client.setClientStatus(READ_END);
		else
			client.setClientStatus(READING);
	}
	else
	{
		if (httpBody.size() >= BODY_LIMIT)
			client.setClientStatus(BODY_LIMIT_OVER);
		else if (httpBody.size() > contentLength)
			client.setClientStatus(BODY_SIZE_OVER); // Error 400?
		else if (httpBody.size() == contentLength)
			client.setClientStatus(READ_END);
		else if (httpBody.size() < contentLength)
			client.setClientStatus(READING);
		else {}
			// client.setClientStatus(ERROR_400);
	}
}

static void	purifyHttpBody(Client& client)
{
	Request&	request = client.getClientRequest();

	request.handleChunkedBody();
	request.handleMultipartBody();
//
//	std::vector<Content> contents = request.getContents();
//	for (std::vector<Content>::iterator cit = contents.begin();
//		cit != contents.end(); ++cit)
//	{
//		std::cout << "[" << cit->name << "]" << std::endl;
//		std::cout << "[" << cit->filename << "]" << std::endl;
//		std::cout << "[" << cit->type << "]" << std::endl;
//		std::cout << "[" << cit->data  << "]" << std::endl;
//	}
}

/**
 * Utility Function
 */

static void	throwError(std::string const& msg)
{
	std::string	errorMessage = msg + ": " + strerror(errno);

	throw std::runtime_error(errorMessage);
}
