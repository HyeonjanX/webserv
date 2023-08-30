/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gychoi <gychoi@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/28 16:02:04 by gychoi            #+#    #+#             */
/*   Updated: 2023/08/30 16:56:16 by gychoi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

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

void	change_events(std::vector<struct kevent>& change_list, uintptr_t ident, int16_t filter,
			uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
	struct kevent	temp_event;

	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	change_list.push_back(temp_event);
}

void	disconnect_client(int client_fd, std::map<int, std::string>& clients)
{
	std::cout << "client disconnected: " << client_fd << std::endl;
	close(client_fd);
	clients.erase(client_fd);
}

/*
https://novice-programmer-story.tistory.com/40
https://jhnyang.tistory.com/251
https://velog.io/@augus-xury/webservHTTP-%EC%84%9C%EB%B2%84-%EB%A7%8C%EB%93%A4%EA%B8%B0
*/

int	main(void)
{
	int					sockfd;
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
	fcntl(sockfd, F_SETFL, O_NONBLOCK); // 왜 필요할까?

	int	kq;
	if ((kq = kqueue()) == -1)
	{
		close(sockfd);
		error("Cannot create kqueue");
	}

	std::map<int, std::string>		clients;
	std::vector<struct kevent>	change_list;
	struct kevent				event_list[8];

	change_events(change_list, static_cast<uintptr_t>(sockfd), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

	int				new_events;
	struct kevent*	curr_event;
	while (true)
	{
		new_events = kevent(kq, &change_list[0], static_cast<int>(change_list.size()), event_list, 8, NULL);
		if (new_events == -1)
		{
			close(sockfd);
			error("Cannot create kevent");
		}
		change_list.clear();

		for (int i = 0; i < new_events; ++i)
		{
			curr_event = &event_list[i];
			if (curr_event->flags & EV_ERROR)
			{
				if (static_cast<int>(curr_event->ident) == sockfd)
				{
					close(sockfd);
					error("Server socket error");
				}
				else
				{
					std::cout << "client socket error" << std::endl;
					disconnect_client((int)curr_event->ident, clients);
				}
			}
			else if (curr_event->filter == EVFILT_READ)
			{
				if ((int)curr_event->ident == sockfd)
				{
					int	connfd;

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
					fcntl(connfd, F_SETFL, O_NONBLOCK); // 왜 필요할까?

					change_events(change_list, static_cast<uintptr_t>(connfd), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
					change_events(change_list, static_cast<uintptr_t>(connfd), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
					clients[connfd] = "";
					std::cout << "===== Client connected =====" << std::endl;
				}
				else if (clients.find((int)curr_event->ident) != clients.end())
				{
					char		buffer[1024] = {0, };
					while (true)
					{
						ret = recv((int)curr_event->ident, buffer, sizeof(buffer), 0);
						if (ret == -1)
						{
							close(sockfd);
							disconnect_client((int)curr_event->ident, clients); // 한 번에 모든 클라이언트를 지우러면?
							error("Fatal: recv");
						}
						else if (ret == 0)
						{
							disconnect_client((int)curr_event->ident, clients);
							break;
						}
						buffer[ret] = '\0';
						clients[(int)curr_event->ident].append(buffer);
						if (isAllReceived(clients[(int)curr_event->ident]))
							break;
					}
				}
			}
			else if (curr_event->filter == EVFILT_WRITE)
			{
				std::map<int, std::string>::iterator	it = clients.find((int)curr_event->ident);
				if (it != clients.end())
				{
					if (clients[(int)curr_event->ident] != "")
					{
						if (clients[(int)curr_event->ident].find("HTTP") == std::string::npos)
							error("Malformed http request");
						std::size_t	startPos = clients[(int)curr_event->ident].find("GET ");
						std::size_t	endPos;
						std::string	filename;
						if (startPos != std::string::npos)
						{
							std::cout << "GET RECEIVED" << std::endl;
							startPos += 4;
							endPos = clients[(int)curr_event->ident].find(' ', startPos);
							filename = clients[(int)curr_event->ident].substr(startPos, endPos - startPos);
							if (!filename.empty() && filename[0] == '/')
								filename = filename.substr(1);
							if (filename.empty())
							{
								ret = send((int)curr_event->ident, SERVER_MSG.c_str(), SERVER_MSG.size(), 0);
								if (ret == -1)
								{
									close(sockfd);
									disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
									error("Fatal: send");
								}
							}
							else
							{
								DIR*	dir = opendir(".");
								if (dir == NULL)
								{
									close(sockfd);
									disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
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

											ret = send((int)curr_event->ident, response.c_str(), response.size(), 0);
											if (ret == -1)
											{
												close(sockfd);
												closedir(dir);
												disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
												error("Fatal: send");
											}
										}
										else
										{
											ret = send((int)curr_event->ident, ERROR_MSG.c_str(), ERROR_MSG.size(), 0);
											if (ret == -1)
											{
												close(sockfd);
												closedir(dir);
												disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
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
									ret = send((int)curr_event->ident, ERROR_MSG.c_str(), ERROR_MSG.size(), 0);
									if (ret == -1)
									{
										close(sockfd);
										closedir(dir);
										disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
										error("Fatal: send");
									}
									closedir(dir);
								}
							}
						}
						else if (clients[(int)curr_event->ident].find("POST ") != std::string::npos)
						{
							std::cout << "POST RECEIVED" << std::endl;
							ret = send((int)curr_event->ident, SERVER_MSG.c_str(), SERVER_MSG.size(), 0);
							if (ret == -1)
							{
								close(sockfd);
								disconnect_client((int)curr_event->ident, clients); // 한 번에 모두 해제하려면?
								error("Fatal: send");
							}
						}
					}
				}
			}
		}
		//close(connfd);
	}
	close(sockfd);
	return 0;
}
