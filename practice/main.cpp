/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/09/01 01:29:26 by gychoi           ###   ########.fr       */
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

#include <iostream>
#include <fstream>
#include <string>

#include <map>
#include <vector>

#define PORT 8080
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
	socklen_t			addrlen;
	struct sockaddr_in	addr;
	std::string			data;
};

static int	count = 1;

/***************************************************************************************************
 * Utility Functions
 ***************************************************************************************************/

void	errorExit(std::string const msg)
{
	// 열려있는 파일디스크립터는 닫지 않았다.
	std::cerr << msg << std::endl;
	exit(EXIT_FAILURE);
}

void	change_events(std::vector<struct kevent>& change_list, uintptr_t ident, int16_t filter,
		uint16_t flags, uint32_t fflags, intptr_t data, void* udata)
{
	struct kevent	temp_event;

	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(temp_event);
}

void	disconnect_client(int fd)
{
	std::cout << "Client disconnected: " << fd << std::endl;
	close(fd);
//	clients.erase(fd); // erase가 필요한 이유?
}

int	getContentLength(std::string const& receivedData)
{
	std::string	key = "Content-Length:";
	std::string	value;
	std::size_t	startPos = receivedData.find(key);
	std::size_t	endPos;

	if (startPos != std::string::npos)
	{
		startPos += key.length();
		endPos = receivedData.find("\r\n", startPos);
		if (endPos != std::string::npos)
		{
			value = receivedData.substr(startPos, endPos - startPos);
			return stoi(value); // C++11
		}
	}
	// content-length가 없는 경우?
	return 0;
}

std::string	extractHttpBody(std::string const& httpMessage)
{
	std::string	doubleCRLF = "\r\n\r\n";
	std::size_t	bodyPos = httpMessage.find(doubleCRLF);

	if (bodyPos != std::string::npos)
	{
		bodyPos += doubleCRLF.length();
		return httpMessage.substr(bodyPos);
	}
	return "";
}

bool	isAllReceived(std::string const& receivedData)
{
	int			contentLength = getContentLength(receivedData);
	std::string	httpBody = extractHttpBody(receivedData);

//	std::cout << contentLength << ", " << static_cast<int>(httpBody.length()) << std::endl;
	if (contentLength == static_cast<int>(httpBody.length()))
		return (true);
	else
		return (false);
}

/***************************************************************************************************
 * Setting/Init Functions
 ***************************************************************************************************/

static Server	_setServer(int port)
{
	Server	s = {};

	if (!memset(&s, 0x0, sizeof(s)))
		errorExit("Server Error: memset failed");
	s.sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s.sock == -1)
		errorExit("Server Error: create server socket failed");
	s.port = port;
	s.maxClients = CLIENT_MAX;
	if (!memset(&(s.addr), 0x0, sizeof(s.addr)))
		errorExit("Server Error: memset failed");
	s.addr.sin_family = AF_INET;
	s.addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s.addr.sin_port = htons(s.port);

	int	enable = 1;
	if (setsockopt(s.sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		errorExit("Server Error: setsockopt failed");

	if (bind(s.sock, reinterpret_cast<struct sockaddr*>(&(s.addr)), sizeof(s.addr)) == -1)
		errorExit("Server Error: bind failed");
	if (listen(s.sock, s.maxClients) == -1)
		errorExit("Server Error: listen failed");
	fcntl(s.sock, F_SETFL, O_NONBLOCK);
	return s;
}

static int	_setKqueue(std::vector<Server> server)
{
	int								kq;
	int								retval;
	std::vector<struct kevent>		change_list;
	std::vector<Server>::iterator	iter;

	if ((kq = kqueue()) == -1)
		errorExit("Error: create kqueue failed");
	for (iter = server.begin(); iter != server.end(); iter++)
	{
		std::cout << static_cast<uintptr_t>(iter->sock) << ", " << iter->sock << std::endl;
		change_events(change_list, static_cast<uintptr_t>(iter->sock), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	}
	retval = kevent(kq, &change_list[0], (sizeof(change_list) / sizeof(struct kevent)), 0, 0, 0); // 일단 서버 이벤트는 안받음
	if (retval == -1)
		errorExit("Error: kevent failed");
	std::cout << retval << std::endl;
	return kq;
}

static Client	_connectNewClient(int serverSocket)
{
	Client	c = {};

	if (!memset(&c, 0x0, sizeof(c)))
		errorExit("Client Error: memset failed");
	c.sock = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&(c.addr)), &(c.addrlen));
	if (c.sock == -1)
		errorExit("Client Error: create client socket failed");

	std::cout << "[INFO] : Client Connected, connect count: " << count++ << std::endl;

//	struct linger	lingerOpt = { 1, 0 };
//	if (setsockopt(c->sock, SOL_SOCKET, SO_LINGER, &lingerOpt, sizeof(lingerOpt)) == -1)
//			errorExit("Client Error: setsockopt failed");

//	if (getsockname(c->sock, reinterpret_cast<struct sockaddr*>(&(c->addr)), &(c->addrlen)) == -1)
//		errorExit("Client Error: getsockname failed");
//	c->port = c->addr.sin_port;
	c.port = c.addr.sin_port;
	std::cout << "client port: " << c.port << std::endl;

	fcntl(c.sock, F_SETFL, O_NONBLOCK);
	return c;
}

/***************************************************************************************************
 * Utility Functions 2
 ***************************************************************************************************/

static void	_sendDefaultPage(Client client)
{
	ssize_t		retval;
	std::string	body;
	std::string	header;
	std::string	response;

	body += "<html><head><title>Default Page</title></head><body><h1>Default Page</h1></body></html>";
//	std::cout << "size: " << body.size() << std::endl;
	header += "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html; charset=utf-8\r\n";
	header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	header += "Keep-Alive: timeout=5, max=100\r\n";
	header += "\r\n";
	response = header + body;

	retval = send(client.sock, response.c_str(), response.size(), 0);
	if (retval == -1)
		errorExit("Error: send failed");
}

static void	_sendErrorPage(Client client, std::string const message)
{
	ssize_t		retval;
	std::string	body;
	std::string	header;
	std::string	response;

	body += "<html><head><title>Error Page</title></head><body><h1>" + message + "</h1></body></html>";
	header += "HTTP/1.1 400 Bad Request\r\n";
	header += "Content-Type: text/html; charset=utf-8\r\n";
	header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	header += "Keep-Alive: timeout=5, max=100\r\n";
	header += "\r\n";
	response = header + body;

	retval = send(client.sock, response.c_str(), response.size(), 0);
	if (retval == -1)
		errorExit("Error: send failed");
}

/***************************************************************************************************
 * Main Functions
 ***************************************************************************************************/

static void	_readClientData(Client& client)
{
	ssize_t		retval;
	char		buffer[BUFFER_SIZE] = { 0, };

	retval = recv(client.sock, buffer, sizeof(buffer), 0);
	if (retval == -1)
		errorExit("Error: recv failed");
	else if (retval == 0)
		return;
	else
	{
		client.data.append(buffer, static_cast<std::size_t>(retval));
		if (isAllReceived(client.data))
			return;
	}
}

static int	_verifyClientData(std::string data)
{
	std::size_t	startPos;
	std::size_t	endPos;
	std::string	filename;

//	std::cout << data << std::endl;
	if (data.find("HTTP") == std::string::npos)
		return -1;
	if (data.find("GET ") != std::string::npos)
	{
		startPos = data.find("GET ") + 4;
		endPos = data.find(' ', startPos);
		filename = data.substr(startPos, endPos - startPos);
		if (!filename.empty() && filename[0] == '/')
			return 1;
		else
			return -1;
	}
	else if (data.find("POST ") != std::string::npos)
	{
		return 2;
	}
	return 3;
}

//static std::string&	_readFile(std::string const filename, std::string& data)
//{
//	std::string		line = "";
//	std::ifstream	file(filename.c_str());
//
//	while(std::getline(file, line))
//		data.append(line);
//	return data;
//}

static void	_sendFile(Client client, std::string filename)
{
	DIR*			dir;
	struct dirent*	entry;
	ssize_t			retval;
	std::vector<char>		body;
	std::string		header = "";
	std::string		response = "";

	dir = opendir(".");
	if (dir == NULL)
		errorExit("Server Error: opendir failed");
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_REG && std::string(entry->d_name) == filename)
		{
			std::ifstream	file(entry->d_name, std::ios::binary);
			if (file.is_open())
			{
				//_readFile(filename, body);
				body = std::vector<char>((std::istreambuf_iterator<char>(file)), \
					std::istreambuf_iterator<char>());
				file.close();
			}
			else
			{
				std::cout << "[INFO] : Cannot open " << filename << std::endl;
				_sendErrorPage(client, "Open file failed");
				closedir(dir);
				return ;
			}
			header += "HTTP/1.1 200 OK\r\n";
			if (filename.find("jpg") != std::string::npos)
				header += "Content-Type: image/jpeg; charset=utf-8\r\n";
			else
				header += "Content-Type: text/html; charset=utf-8\r\n";
			header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
			header += "Keep-Alive: timeout=5, max=100\r\n";
			header += "\r\n";
			//response = header + body;
			response = header + std::string(body.begin(), body.end());
			retval = send(client.sock, response.c_str(), response.size(), 0);
			if (retval == -1)
				errorExit("Error : send failed");
			closedir(dir);
			break;
		}
	}
	if (entry == NULL)
	{
		std::cout << "[INFO] : File not found: " << filename << std::endl;
		_sendErrorPage(client, "File not found");
		closedir(dir);
	}
}

static void	_handleGetRequest(Client client)
{
	std::size_t	startPos;
	std::size_t endPos;
	std::string	filename;

	std::cout << "[INFO] : GET Request Received" << std::endl;
	startPos = client.data.find("GET ") + 4;
	endPos = client.data.find(' ', startPos);
	filename = (client.data.substr(startPos, endPos - startPos)).substr(1);
	if (filename.empty())
		_sendDefaultPage(client);
	else
		_sendFile(client, filename);
}

//static void	_handlePostRequest(Client client)
//{
//	std::cout << "[INFO] : POST Request Received" << std::endl;
//	_sendDefaultPage(client);
//}

static int	_getEvents(int kq, struct kevent* evList)
{
	int	retval;
	struct timespec	timeout;

	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;
	retval = kevent(kq, 0, 0, evList, (sizeof(*evList) / sizeof(evList[0])), &timeout);
	if (retval == -1)
		errorExit("Error: kevent failed");
	std::cout << retval << std::endl;
	return retval;
}

static void	_runServer(std::vector<Server> serverVec, int kq)
{
	Client									client;
	std::vector<struct Client>				clientVec;
	std::vector<struct Client>::iterator	iterC;
	int										eventNum;
	int										retval;
	struct kevent							evList[8];
	struct kevent*							currEvent;
	std::vector<struct kevent>				change_list;
	std::vector<struct Server>::iterator	iterS;

	while (true)
	{
		eventNum = _getEvents(kq, evList);
		for (int i = 0; i < eventNum; i++)
		{
			currEvent = &evList[i];
			if (currEvent->flags & EV_ERROR)
			{
				for (iterS = serverVec.begin(); iterS != serverVec.end(); iterS++)
				{
					if (currEvent->ident == static_cast<uintptr_t>(iterS->sock))
						std::cout << "[INFO] : Server error on " << iterS->sock << std::endl;
					else
					{
						std::cout << "[INFO] : Client error on " << iterS->sock << std::endl;
						disconnect_client(static_cast<int>(currEvent->ident));
						for (iterC = clientVec.begin(); iterC != clientVec.end(); iterC++)
						{
							if (currEvent->ident == static_cast<uintptr_t>(iterC->sock))
								clientVec.erase(iterC--);
						}
					}
				}
			}
			else if (currEvent->filter == EVFILT_READ)
			{
				for (iterS = serverVec.begin(); iterS != serverVec.end(); iterS++)
				{
					if (currEvent->ident == static_cast<uintptr_t>(iterS->sock))
					{
						client = _connectNewClient(iterS->sock);
						change_events(change_list, static_cast<uintptr_t>(client.sock), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
						change_events(change_list, static_cast<uintptr_t>(client.sock), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
						clientVec.push_back(client);
						break;
					}
					for (iterC = clientVec.begin(); iterC != clientVec.end(); iterC++)
					{
						if (currEvent->ident == static_cast<uintptr_t>(iterC->sock))
						{
							_readClientData(*iterC);
							retval = _verifyClientData(iterC->data);
							if (retval == -1)
								std::cout << "[INFO] : Malformed HTTP Data" << std::endl;
							else if (retval == 1)
								_handleGetRequest(client);
//							else if (retval == 2)
//								_handlePostRequest(client);
							else
								std::cout << "[INFO] : Other HTTP Request Received" << std::endl;
							break;
						}
					}
				}
			}
			else if (currEvent->filter == EVFILT_WRITE)
			{
				for (iterC = clientVec.begin(); iterC != clientVec.end(); iterC++)
				{
					if (currEvent->ident == static_cast<uintptr_t>(iterC->sock))
					{
						std::cout << "[INFO] : Client " << iterC->sock << " gets WRITE event!" << std::endl;
						break;
					}
				}
			}
		}
	}
	// need to clean up all server, client fds
	//close(server.sock);
}

int	main(void)
{
	int							kq;
	std::vector<Server>			server;

	server.push_back(_setServer(8080));
	server.push_back(_setServer(8081));
	server.push_back(_setServer(8082));
	kq = _setKqueue(server);
	_runServer(server, kq);
	return 0;
}
