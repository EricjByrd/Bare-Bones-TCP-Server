// TCP_Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
int main()
{
    //Instantiating the Socket
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int err;

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    printf("////////////////////////////////////////////////////\n"
        "///////Welcome to your home TCP Server//////////////\n"
        "////////////////////////////////////////////////////\n\n");

    printf("Waiting for client...\n\n");

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    //Building the socket out
    sockaddr_in serverAddress = { 0 };

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, SOMAXCONN);
    while (1) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            printf("Connection to Client failed %d\n", WSAGetLastError());
        }
        else {
            char buffer[1024] = { 0 };
            recv(clientSocket, buffer, sizeof(buffer), 0);
            printf("Message from client: %s\n", buffer);
            closesocket(clientSocket);
        }
    }
    closesocket(serverSocket);
    WSACleanup();
}
