/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/08/29 22:35:04 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/events.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>

std::string	SERVER_MSG = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 78\r\nConnection: close\r\n\r\n<html><head><title>Default Page</title></head><body>Default page</body></html>";
std::string	ERROR_MSG = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 75\r\nConnection: close\r\n\r\n<html><head><title>ERROR</title></head><body>Cannot open file</body></html>";

void	error(std::string const msg)
{
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

	std::cout << contentLength << ", " << static_cast<int>(httpBody.length()) << std::endl;
	if (contentLength == static_cast<int>(httpBody.length()))
		return (true);
	else
		return (false);
}

/*
https://novice-programmer-story.tistory.com/40
https://jhnyang.tistory.com/251
https://velog.io/@augus-xury/webservHTTP-%EC%84%9C%EB%B2%84-%EB%A7%8C%EB%93%A4%EA%B8%B0
*/

int	main(void)
{
	int					sockfd, connfd;
	ssize_t				ret;
	socklen_t			addrlen;
	struct sockaddr_in	serverAddr, clientAddr;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1)
		error("Cannot create socket");
	if (!memset(&serverAddr, 0x00, sizeof(serverAddr)))
		error("Fatal: memset");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(8080);
	if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
	{
		close(sockfd);
		error("Cannot bind socket");
	}
	if (listen(sockfd, 5) == -1)
	{
		close(sockfd);
		error("Cannot listen socket");
	}

	while (true)
	{
		connfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrlen);
		if (connfd == -1)
		{
			close(sockfd);
			error("Cannot accept socket");
		}
		struct linger	lingerOpt = { 1, 0 };
		if (setsockopt(connfd, SOL_SOCKET, SO_LINGER, &lingerOpt, sizeof(lingerOpt)) == -1)
		{
			close(sockfd);
			close(connfd);
			error("Fatal: setsockopt");
		}
		std::cout << "===== Client connected =====" << std::endl;

		char		buffer[1024] = {0, };
		std::string	receivedData = "";
		while (true)
		{
			ret = recv(connfd, buffer, sizeof(buffer), 0);
			if (ret == -1)
			{
				close(sockfd);
				close(connfd);
				error("Fatal: recv");
			}
			else if (ret == 0)
				break;
			buffer[ret] = '\0';
			receivedData.append(buffer);
			if (isAllReceived(receivedData))
				break;
		}

		if (receivedData.find("HTTP") == std::string::npos)
			error("Malformed http request");
		std::size_t	startPos = receivedData.find("GET ");
		std::size_t	endPos;
		std::string	filename;
		if (startPos != std::string::npos)
		{
			std::cout << "GET RECEIVED" << std::endl;
			startPos += 4;
			endPos = receivedData.find(' ', startPos);
			filename = receivedData.substr(startPos, endPos - startPos);
			if (!filename.empty() && filename[0] == '/')
				filename = filename.substr(1);
			if (filename.empty())
			{
				ret = send(connfd, SERVER_MSG.c_str(), SERVER_MSG.size(), 0);
				if (ret == -1)
				{
					close(sockfd);
					close(connfd);
					error("Fatal: send");
				}
			}
			else
			{
				DIR*	dir = opendir(".");
				if (dir == NULL)
				{
					close(sockfd);
					close(connfd);
					error("Cannot open directory");
				}
				struct dirent*	entry;
				while ((entry = readdir(dir)) != NULL)
				{
					if (entry->d_type == DT_REG && std::string(entry->d_name) == filename)
					{
						std::string		body = "";
						std::ifstream	file(entry->d_name);
						if (file.is_open())
						{
							std::string line;
							while (std::getline(file, line))
								body.append(line);
							file.close();
							std::string	response = "HTTP/1.1 200 OK\r\n";
							response += "Content-Type: text/html; charset=utf-8\r\n";
							response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
							response += "Keep-Alive: timeout=5, max=100\r\n";
							response += "\r\n" + body;

							ret = send(connfd, response.c_str(), response.size(), 0);
							if (ret == -1)
							{
								close(sockfd);
								close(connfd);
								closedir(dir);
								error("Fatal: send");
							}
						}
						else
						{
							ret = send(connfd, ERROR_MSG.c_str(), ERROR_MSG.size(), 0);
							if (ret == -1)
							{
								close(sockfd);
								close(connfd);
								closedir(dir);
								error("Fatal: send");
							}
						}
						closedir(dir);
						break;
					}
				}
				if (entry == NULL)
				{
					std::cout << "File not found: " << filename << std::endl;
					ret = send(connfd, ERROR_MSG.c_str(), ERROR_MSG.size(), 0);
					if (ret == -1)
					{
						close(sockfd);
						close(connfd);
						closedir(dir);
						error("Fatal: send");
					}
					closedir(dir);
				}
			}
		}
		else if (receivedData.find("POST ") != std::string::npos)
		{
			std::cout << "POST RECEIVED" << std::endl;
			ret = send(connfd, SERVER_MSG.c_str(), SERVER_MSG.size(), 0);
			if (ret == -1)
			{
				close(sockfd);
				close(connfd);
				error("Fatal: send");
			}
		}
		//close(connfd);
	}
	close(sockfd);
	return 0;
}
