#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>        // Needed for _beginthread() and _endthread()

// Link with ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

void recvThread(void* sock) {
    char msgbuf[1204];
    SOCKET RecvSocket = (SOCKET)sock;
    struct sockaddr_in si_other;
    int addrlen = sizeof(si_other);
    int recvlen = 0;
    char ipstringbuffer[46];
    memset(ipstringbuffer, 0, 46);

    while (1) {
        memset(msgbuf, 0, 1024);
        if ((recvlen = recvfrom(RecvSocket, msgbuf, 1024, 0, (struct sockaddr*)&si_other, &addrlen)) == SOCKET_ERROR) {
            printf("recvfrom() fail with error code: %d\n", WSAGetLastError());
            exit(EXIT_FAILURE);
        }
        inet_ntop(AF_INET, &si_other.sin_addr, ipstringbuffer, 46);
        printf("Receive from %s\n", ipstringbuffer);
        printf("Data: %s\n", msgbuf);
        printf("Enter Sentence:\n");

        if (strcmp(msgbuf, "quit") == 0)
            break;
    }
    closesocket(RecvSocket);
    _endthread();
}

int main(int argc, char** argv)
{
    unsigned short Port = 27015;

    if (argc != 4) {
        return 1;
    }
    int port = atoi(argv[1]);
    char* address = argv[2];
    int toport = atoi(argv[3]);
    printf("本地端口：%d\n远程地址：%s\n远程端口：%d\n", port, address, toport);

    int iResult;
    WSADATA wsaData;

    SOCKET SendSocket = INVALID_SOCKET;
    sockaddr_in RecvAddr;

    char SendBuf[1024];
    int BufLen = 1024;

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
    }

    //---------------------------------------------
    // Create a socket for sending data
    SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
    }

    //---------------------------------------------
    // Set up the RecvAddr structure with the IP address of
    // the receiver (in this example case "192.168.1.1")
    // and the specified port number.
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(toport);
    inet_pton(AF_INET, address, &RecvAddr.sin_addr.s_addr);//vs2013版本以上使用新的函数转换IP地址

    /***********************************************************/
    SOCKET RecvSocket;
    struct sockaddr_in server;
    char hostname[NI_MAXHOST];
    struct addrinfo hints;
    struct addrinfo* result = NULL;
    struct sockaddr_in host_addr;
    char ipstringbuffer[46];
    short listen_port;
    char* LISTEN_PORT = argv[3];
    /*
    gethostname(hostname, NI_MAXHOST);
    printf("hostname: %s\n", hostname);
    // Resolve the address and port
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    iResult = getaddrinfo(hostname, LISTEN_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("getaddrinfo done.\n");

    // Display server information
    memcpy(&host_addr, result->ai_addr, sizeof(host_addr));
    inet_ntop(AF_INET, &host_addr.sin_addr, ipstringbuffer, sizeof(ipstringbuffer));
    listen_port = ntohs(host_addr.sin_port);
    printf("Server Address: %s\n", ipstringbuffer);
    printf(" Server Port Number: %d\n", listen_port);*/

    // Open Socket
    if ((RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Create socket %d done.\n", RecvSocket);

    // Bind the socket to any address and the specified port.
    struct sockaddr_in RecvAddr1;
    RecvAddr1.sin_family = AF_INET;
    RecvAddr1.sin_port = htons(port);
    RecvAddr1.sin_addr.s_addr = htonl(INADDR_ANY);

    iResult = bind(RecvSocket, (SOCKADDR*)&RecvAddr1, sizeof(RecvAddr1));
    if (iResult != 0) {
        wprintf(L"bind failed with error %d\n", WSAGetLastError());
        return 1;
    }

    // Open receive thread
    printf("Start open a thread....\n");
    if (_beginthread(recvThread, 4096, (void*)RecvSocket) < 0)
        printf("thread error.\n");

    /*******************************************************/
    while (1) {
        printf("Enter Sentence:\n");
        gets_s(SendBuf);
        // Bail out if "quit" is entered
        if (strcmp(SendBuf, "quit") == 0)
            break;

        iResult = sendto(SendSocket,
            SendBuf, BufLen, 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
        if (iResult == SOCKET_ERROR) {
            wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
            closesocket(SendSocket);
            WSACleanup();
        }
        else
            printf("Message is sent successfully!\n");
    }
    // Clean up and quit.
    wprintf(L"Exit.\n");
    WSACleanup();
    return 0;
}