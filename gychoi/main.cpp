/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/06 19:46:55 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/15 23:14:45 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Client.hpp"

#define CLIENT_MAX 5

struct	Server
{
	int					sock;
	int					port;
	int					maxClients;
	struct sockaddr_in	addr;
};

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

void	throwError(std::string const& msg)
{
	// need to close all fds;
	std::string	errorMessage = msg + ": " + strerror(errno);

	throw std::runtime_error(errorMessage);
}

void	closeServers(std::vector<Server>& servers)
{
	std::vector<Server>::iterator	iter;

	std::cout << "[INFO] : Closing " << servers.size() << " servers"
		<< std::endl;
	for (iter = servers.begin(); iter != servers.end(); ++iter)
	{
		std::cout << "Delete Server fd: " << iter->sock << std::endl;
		close(iter->sock);
	}
}

void	closeClients(std::map<int, Client>& clients)
{
	std::map<int, Client>::iterator	iter;

	std::cout << "[INFO] : Closing " << clients.size() << " clients"
		<< std::endl;
	for (iter = clients.begin(); iter != clients.end(); ++iter)
	{
		std::cout << "Delete Client fd: "
			<< iter->second.getClientSocket() << std::endl;
		close(iter->second.getClientSocket());
	}
}

void	disconnectClient(int fd, int port, std::map<int, Client>& clients)
{
	std::cout << "[INFO] : Client disconnected: " << port << std::endl;
	close(fd);
	clients.erase(fd);
}

void	updateEvents(std::vector<struct kevent>& updateList, uintptr_t ident,
		int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data,
		void *udata)
{
	struct kevent	temp;

	EV_SET(&temp, ident, filter, flags, fflags, data, udata);
	updateList.push_back(temp);
}

/*******************************************************************************
 * Init/Set Functions
 ******************************************************************************/

Server	setServer(int port)
{
	Server	s;
	int		val = 1;

	if (!memset(&s, 0x0, sizeof(s)))
		throwError("Server error: memset");
	if ((s.sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throwError("Server error: socket");
	s.port = port;
	s.maxClients = CLIENT_MAX;
	if (!memset(&s.addr, 0x0, sizeof(s.addr)))
	{
		close(s.sock);
		throwError("Server error: memset");
	}
	s.addr.sin_family = AF_INET;
	s.addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s.addr.sin_port = htons(s.port);
	if (setsockopt(s.sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1)
	{
		close(s.sock);
		throwError("Server error: setsockopt");
	}
	if (bind(s.sock, reinterpret_cast<struct sockaddr*>(&s.addr),
				sizeof(s.addr)) == -1)
	{
		close(s.sock);
		throwError("Server error: bind");
	}
	if (listen(s.sock, s.maxClients) == -1)
	{
		close(s.sock);
		throwError("Server error: listen");
	}
	if (fcntl(s.sock, F_SETFL, O_NONBLOCK) == -1)
	{
		close(s.sock);
		throwError("Server error: fcntl");
	}
	return s;
}

int	setKqueue(void)
{
	int	kq;

	if ((kq = kqueue()) == -1)
		throwError("Kqueue error: kqueue");
	return kq;
}

/*******************************************************************************
 * Main Functions
 ******************************************************************************/

// handle Read Event는 Client 객체를 포함하는 Webserve 내에서
// 진행하는 것이 좋다고 생각했다.
void	handleReadEvent(struct kevent* currEvent,
		std::vector<struct kevent>& updateList,
		std::vector<Server>& servers, std::map<int, Client>& clients)
{
	for (std::vector<Server>::iterator sit = servers.begin();
			sit != servers.end(); ++sit)
	{
		if (sit->sock == static_cast<int>(currEvent->ident))
		{
			Client	client(sit->sock);
			updateEvents(updateList,
					static_cast<uintptr_t>(client.getClientSocket()),
					EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
			updateEvents(updateList,
					static_cast<uintptr_t>(client.getClientSocket()),
					EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, 0);
			clients.insert(std::make_pair(client.getClientSocket(), client));
			return ;
		}
	}
	for (std::map<int, Client>::iterator cit = clients.begin();
			cit != clients.end(); ++cit)
	{
		if (cit->second.getClientSocket() != static_cast<int>(currEvent->ident))
			continue;
		if (currEvent->flags & EV_EOF)
		{
			disconnectClient(cit->second.getClientSocket(),
							cit->second.getClientPort(), clients);
			break;
		}
		if (!cit->second.readClientRequest()
			|| cit->second.getClientStatus() == ERROR_400)
		{
			disconnectClient(cit->second.getClientSocket(),
							cit->second.getClientPort(), clients);
			break;
		}
		if (cit->second.getClientStatus() == BODY_LIMIT_OVER
			|| cit->second.getClientStatus() == BODY_SIZE_OVER)
		{
			// 우선은 READ_END와 동작이 동일하지만 추가적인 처리가 필요
			std::cout << "BODY_LIMIT_OVER || BODY_SIZE_OVER" << std::endl;

			updateEvents(updateList, static_cast<uintptr_t>
			(cit->second.getClientSocket()),
			EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, 0);
			updateEvents(updateList, static_cast<uintptr_t>
			(cit->second.getClientSocket()),
			EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
			break;
		}
		else if (cit->second.getClientStatus() == READ_END)
		{
			std::cout << "READ_END" << std::endl;

			if (cit->second.getClientRequest().getHttpMethod() == "POST")
			{
				if (!cit->second.uploadClientFile())
				{
					disconnectClient(cit->second.getClientSocket(),
									cit->second.getClientPort(), clients);
					break;
				}
			}
			updateEvents(updateList, static_cast<uintptr_t>
			(cit->second.getClientSocket()),
			EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, 0);
			updateEvents(updateList, static_cast<uintptr_t>
			(cit->second.getClientSocket()),
			EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
			break;
		}
	}
}

std::string	tempResponseException(short status)
{
	std::string	header;
	std::string	body;

	if (status == BODY_LIMIT_OVER)
	{
		header += "HTTP/1.1 413 Payload Too Large\r\n";
		body += "413 Payload Too Large";
	}
	else if (status == BODY_SIZE_OVER)
	{
		header += "HTTP/1.1 400 Bad Request\r\n";
		body += "400 Bad Request";
	}
	header += "Content-Type: text/html; charset=utf-8\r\n";
	header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	header += "\r\n";
	return header + body;
}

bool	writeClientData(Client& client)
{
	ssize_t		sendByte;
	std::string	header;
	std::string	body;
	std::string	response;

	if (client.getClientStatus() == READ_END)
	{
		if (client.getClientRequest().getHttpMethod() == "GET")
		{
			body = client.getClientRequest().getHttpBody();
			header += "HTTP/1.1 200 OK\r\n";
		}
		else if (client.getClientRequest().getHttpMethod() == "POST")
		{
			body = "upload success";
			header += "HTTP/1.1 201 Created\r\n";
		}
		header += "Content-Type: text/html; charset=utf-8\r\n";
		header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		header += "\r\n";
		response = header + body;
	}
	else
		response = tempResponseException(client.getClientStatus());

	if ((sendByte = send(client.getClientSocket(), response.c_str(),
				response.size(), 0)) == -1)
		throwError("send failed");
	else if (sendByte == 0
			|| client.getClientStatus() == BODY_LIMIT_OVER
			|| client.getClientStatus() == BODY_SIZE_OVER) // post 요청도 추가
		return false; // MAY close the connection
	return true;
}

void	handleWriteEvent(struct kevent* currEvent,
		std::vector<struct kevent>& updateList,
		std::map<int, Client>& clients)
{
	for (std::map<int, Client>::iterator cit = clients.begin();
			cit != clients.end(); ++cit)
	{
		if (cit->second.getClientSocket() == static_cast<int>(currEvent->ident))
		{
			if (!writeClientData(cit->second))
				disconnectClient(cit->second.getClientSocket(),
						cit->second.getClientPort(), clients);
			else
			{
				cit->second.getClientRequest().resetRequest();
				updateEvents(updateList, static_cast<uintptr_t>
				(cit->second.getClientSocket()),
				EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
				updateEvents(updateList, static_cast<uintptr_t>
				(cit->second.getClientSocket()),
				EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, 0);
			}
			break;
		}
	}
}

void	runServer(int kq, std::vector<struct kevent>& updateList,
		std::vector<Server>& servers, std::map<int, Client>& clients)
{
	int						newEvents;
	struct kevent			eventList[8];
	struct kevent*			currEvent;

	while (true)
	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::cout << "[INFO] : Connect clients: " << clients.size()
			<< std::endl;

		if ((newEvents = kevent(kq, &updateList[0],
			static_cast<int>(updateList.size()), eventList, 8, 0)) == -1)
			throwError("Kqueue error: kevent");
		updateList.clear();

		for (int i = 0; i < newEvents; ++i)
		{
			currEvent = &eventList[i];
			if (currEvent->flags & EV_ERROR)
			{
				for (std::vector<Server>::iterator sit = servers.begin();
					sit != servers.end(); ++sit)
				{
					if (sit->sock == static_cast<int>(currEvent->ident))
						throwError("Server Error in kevent");
				}
				for (std::map<int, Client>::iterator cit = clients.begin();
					cit != clients.end(); ++cit)
				{
					if (cit->second.getClientSocket()
						== static_cast<int>(currEvent->ident))
					{
						disconnectClient(cit->second.getClientSocket(),
								cit->second.getClientPort(), clients);
						break;
					}
				}
			}
			else if (currEvent->filter == EVFILT_READ)
			{
				std::cout << "READ EVENT" << std::endl;
				handleReadEvent(currEvent, updateList, servers, clients);
			}
			else if (currEvent->filter == EVFILT_WRITE)
			{
				std::cout << "WRITE EVENT" << std::endl;
				handleWriteEvent(currEvent, updateList, clients);
			}
		}
	}
	throwError("Unexpected loop break");
}

int	main(void)
{
	std::vector<Server>			servers;
	std::map<int, Client>		clients;
	int							kq;
	std::vector<struct kevent>	updateList;

	try
	{
		servers.push_back(setServer(8080));
		servers.push_back(setServer(8081));
		servers.push_back(setServer(8082));
		kq = setKqueue();
		for (std::vector<Server>::iterator iter = servers.begin();
			iter != servers.end(); ++iter)
			updateEvents(updateList, static_cast<uintptr_t>(iter->sock),
						EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
		runServer(kq, updateList, servers, clients);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		closeServers(servers);
		closeClients(clients);
	}
	return 0;
}
