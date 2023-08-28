/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/08/28 23:16:16 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>

std::string	SERVER_MSG = "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nContent-Length: 4242\n\n<h1>Hello</h1>";

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
			if (value.empty())
				return -1;
			else
				return stoi(value);
		}
	}
	return -1;
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

		if (receivedData.find("HTTP/") == std::string::npos)
			error("Malformed http request");
		if (receivedData.find(" /GET") != std::string::npos)
		{
			
https://novice-programmer-story.tistory.com/40
https://jhnyang.tistory.com/251
https://velog.io/@augus-xury/webservHTTP-%EC%84%9C%EB%B2%84-%EB%A7%8C%EB%93%A4%EA%B8%B0


		}
		else if (receivedData.find(" /POST") != std::string::npos)
		{
			ret = send(connfd, SERVER_MSG.c_str(), SERVER_MSG.size(), 0);
			if (ret == -1)
			{
				close(sockfd);
				close(connfd);
				error("Fatal: send");
			}
			std::cout << "====== Message send =====" << std::endl;
		}
		close(connfd);
	}
	close(sockfd);
	return 0;
}
