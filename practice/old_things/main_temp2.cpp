/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/08/31 21:58:32 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>

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

void	errorExit(std::string const msg)
{
	// 열려있는 파일디스크립터는 닫지 않았다.
	std::cerr << msg << std::endl;
	exit(EXIT_FAILURE);
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
			return stoi(value);
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

static Server	_setServer(void)
{
	Server	s = {};

	if (!memset(&s, 0x0, sizeof(s)))
		errorExit("Server Error: memset failed");
	s.sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s.sock == -1)
		errorExit("Server Error: create server socket failed");
	s.port = PORT;
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
	return s;
}

static Client	_connectNewClient(int serverSocket)
{
	Client	c = {};

	if (!memset(&c, 0x0, sizeof(c)))
		errorExit("Client Error: memset failed");
	c.sock = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&(c.addr)), &(c.addrlen));
	if (c.sock == -1)
		errorExit("Client Error: create client socket failed");

//	struct linger	lingerOpt = { 1, 0 };
//	if (setsockopt(c->sock, SOL_SOCKET, SO_LINGER, &lingerOpt, sizeof(lingerOpt)) == -1)
//			errorExit("Client Error: setsockopt failed");

//	if (getsockname(c->sock, reinterpret_cast<struct sockaddr*>(&(c->addr)), &(c->addrlen)) == -1)
//		errorExit("Client Error: getsockname failed");
//	c->port = c->addr.sin_port;
	c.port = c.addr.sin_port;
	std::cout << "client port: " << c.port << std::endl;
	return c;
}

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

static void	_handlePostRequest(Client client)
{
	std::cout << "[INFO] : POST Request Received" << std::endl;
	_sendDefaultPage(client);
}

static void	_runServer(Server server)
{
	Client		client;
	int			retval;

	while (true)
	{
		client = _connectNewClient(server.sock);
		std::cout << "[INFO] : Client Connected, connect count: " << count++ << std::endl;
		_readClientData(client);
		retval = _verifyClientData(client.data);
		if (retval == -1)
			std::cout << "[INFO] : Malformed HTTP Data" << std::endl;
		else if (retval == 1)
			_handleGetRequest(client);
		else if (retval == 2)
			_handlePostRequest(client);
		else
			std::cout << "[INFO] : Other HTTP Request Received" << std::endl;
	}
	close(server.sock);
}

int	main(void)
{
	Server	server;

	server = _setServer();
	_runServer(server);
	return 0;
}
