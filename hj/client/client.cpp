#include <iostream>
#include <sstream>
#include <string>
#include <cstring> // For memset()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> // For close()

int main()
{
    
    // localhost:80에 요청 찔러보는 코드 => nginx랑 했었음
    std::string host_name = "localhost";
    int port = 80;



    // Prepare the request line, headers, and body
    std::string request_line = "GET / HTTP/1.1\r\n";
    std::string headers = "Host: localhost\r\nUser-Agent: C++ HTTP Client\r\nAccept-Language: en-US\r\naaaaen;\r\n";
    std::string body = "";
    std::string http_request = request_line + headers + "\r\n" + body;

    std::cout << "Create a socket" << std::endl;
    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // int flags = fcntl(client_socket, F_GETFL, 0);
    // fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    std::cout << "Get the server details" << std::endl;
    // Get the server details
    struct hostent *server = gethostbyname(host_name.c_str());
    if (server == NULL)
    {
        std::cerr << "Error, no such host" << std::endl;
        return -1;
    }

    std::cout << "Fill in the server address" << std::endl;
    // Fill in the server address
    struct sockaddr_in server_address;
    std::memset((char *)&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    std::memcpy((char *)&server_address.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    server_address.sin_port = htons(port);

    std::cout << "Connect to the server" << std::endl;
    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        std::cerr << "Error connecting to server" << std::endl;
        return -1;
    }

    std::cout << "Send the HTTP request" << std::endl;
    // Send the HTTP request
    if (send(client_socket, http_request.c_str(), http_request.length(), 0) < 0)
    {
        std::cerr << "Error sending the request" << std::endl;
        return -1;
    }

    std::cout << "=========================== Receive the response ===========================" << std::endl;
    // Receive the response
    char buffer[4096];
    std::memset(buffer, 0, 4096);

    int received_bytes = 0;
    std::string data;

    while (data.length() != 853 && (received_bytes = recv(client_socket, buffer, 4096 - 1, 0)) > 0)
    {
        data.append(buffer, received_bytes);
        std::cout << "----------------------------- 데이터 -----------------------------" << std::endl;
        std::cout << "data: " << received_bytes << "bytes 읽음, " << "total:" << data.length() << "bytes" << std::endl;
        std::cout << data << std::endl;
        std::cout << "------------------------------------------------------------------" << std::endl;
    }
    if (received_bytes < 0)
    {
        std::cerr << "Error receiving response" << std::endl;
    }
    else
    {
        std::cout << "=========================== 서버 응답받은것 ===========================" << std::endl;
        std::cout << "data: " << data << std::endl;
        std::cout << "================================================================" << std::endl;
    }    

    // Close the socket
    close(client_socket);

    std::cout << "=========================== ********************* ===========================" << std::endl;

    return 0;
}
