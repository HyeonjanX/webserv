/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/01 23:15:34 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>

#include <map>
#include <vector>

#define CLIENT_MAX 5
#define BUFFER_SIZE 1024

struct	Server
{
	int					sock;
	int					port;
	int					maxClients;
	struct sockaddr_in	addr;
};

struct	Client
{
	int					sock;
	int					port;
	struct sockaddr_in	addr;
	socklen_t			addrlen;
	char				buffer[BUFFER_SIZE];
	std::string			data;
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

	for (iter = servers.begin(); iter != servers.end(); ++iter)
	{
		std::cout << "Delete Server fd: " << iter->sock << std::endl;
		close(iter->sock);
	}
}

void	closeClients(std::map<int, Client>& clients)
{
	std::map<int, Client>::iterator	iter;

	for (iter = clients.begin(); iter != clients.end(); ++iter)
	{
		std::cout << "Delete Client fd: " << iter->second.sock << std::endl;
		close(iter->second.sock);
	}
}

void	disconnectClient(int fd, std::map<int, Client>& clients)
{
	std::cout << "[INFO] : Client disconnected: " << fd << std::endl;
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

Client	setClient(int serverSocket)
{
	Client	c;

	if (!memset(&c, 0x0, sizeof(c)))
		throwError("Client error: memset");
	if ((c.sock = accept(serverSocket, 
		reinterpret_cast<struct sockaddr*>(&c.addr), sizeof(c.addr))) == -1)
		throwError("Client error: accept");
	// 여기서부터
}

/*******************************************************************************
 * Main Functions
 ******************************************************************************/

void	handleReadEvent(struct kevent* currEvent,
		std::vector<struct kevent>& updateList,
		std::vector<Server> servers, std::map<int, Client> clients)
{
	Client	client;

	for (std::vector<Server>::iterator sit = servers.begin();
		sit != servers.end(); ++sit)
	{
		if (sit->sock == static_cast<int>(currEvent->ident))
			throwError("Server Error in kevent");
	}
}

void	handleWriteEvent(struct kevent* currEvent,
		std::vector<struct kevent> updateList,
		std::vector<Server> servers, std::map<int, Client> clients)
{
	(void)updateList;
	(void)servers;
	(void)clients;
	std::cout << "Write Event! " << currEvent->ident << std::endl;
}

void	runServer(int kq, std::vector<struct kevent> updateList,
		std::vector<Server> servers, std::map<int, Client> clients)
{
	int						newEvents;
	struct kevent			eventList[8];
	struct kevent*			currEvent;

	(void)servers;
	(void)clients;
	(void)currEvent;
	while (true)
	{
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
					if (cit->second.sock == static_cast<int>(currEvent->ident))
						disconnectClient(cit->second.sock, clients);
				}
			}
			else if (currEvent->filter == EVFILT_READ)
				handleReadEvent(currEvent, updateList, servers, clients);
			else if (currEvent->filter == EVFILT_WRITE)
				handleWriteEvent(currEvent, updateList, servers, clients);
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
