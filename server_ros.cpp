// server.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create listening socket
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Setup the server address structure
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000); // Port number
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on any available network interface

    // Bind the socket
    result = bind(listen_socket, (sockaddr *)&server_addr, sizeof(server_addr));
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    result = listen(listen_socket, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for a connection..." << std::endl;

    // Accept an incoming connection
    SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
    if (client_socket == INVALID_SOCKET)
    {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;

    while(1){
    // Receive data from the client
    float data[3];
    result = recv(client_socket, (char *)data, sizeof(data), 0);
    if (result > 0)
    {
        std::cout << "Received floats: [" << data[0] << ", " << data[1] << ", " << data[2] << "]" << std::endl;
    }
    else if (result == 0)
    {
        std::cout << "Connection closed" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
    }
    }
    // Cleanup
    closesocket(client_socket);
    closesocket(listen_socket);
    WSACleanup();

    return 0;
}
