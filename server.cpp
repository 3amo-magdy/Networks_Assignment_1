#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <Winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


int main(int argc, char const *argv[])
{
    const char* port = argv[0];
    WSADATA wsaData;
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    //retrieve the local address and port to be used by the server (to help in biding the socket)
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    //create socket
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
    }
    // Setup the TCP listening socket (bind the socket)
    iResult = bind(ListenSocket, result->ai_addr, (int)(result->ai_addrlen));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    //free un-needed memory
    freeaddrinfo(result);
    //listen
    if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        printf( "Listen failed with error: %ld\n", WSAGetLastError() );
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    //accept
    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    #define DEFAULT_BUFLEN 512

        closesocket(ListenSocket);

    char recvbuf[DEFAULT_BUFLEN];
    int iResult, iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Receive until the peer shuts down the connection
    do {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

            // Echo the buffer back to the sender
            iSendResult = send(ClientSocket, recvbuf, iResult, 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        } else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    }
    while (iResult > 0);
    // shutdown
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }


    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}
