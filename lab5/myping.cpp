#include <WinSock2.h>
#include <iostream>
#include <ws2tcpip.h>
#include <stdio.h>
#undef UNICODE

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

typedef struct icmp_hdr {
    unsigned char	icmp_type;
    unsigned char	icmp_code;
    unsigned short	icmp_checksum;
    unsigned short	icmp_id;
    unsigned short	icmp_sequence;
} ICMP_HDR;

#define DEFAULT_SIZE            32
#define DEFAULT_RECV_TIMEOUT    4000
#define DEFAULT_TTL             64
#define MAX_RECV_BUF_LEN        0xFFFF
#define DEFAULT_PORT            "0"

using namespace std;

USHORT checksum(USHORT* buffer, int size);
void ComputeIcmpchecksum(char* buf, int packetlen);
void SetIcmpSequence(char* buf);
int	PrintAddress(struct sockaddr* sa, int salen);
struct addrinfo* ResolveAddress(char* addr, int af, int type, int proto);
int RecvFrom(SOCKET s, char* buf, int buflen, SOCKADDR* from, int* fromlen, WSAOVERLAPPED* ol);

int __cdecl main(int argc, char* argv[]) {
    WSADATA wsaData;
    int iResult;
    char* destHost = NULL;
    struct addrinfo* remote = NULL;
    struct addrinfo* local = NULL;
    SOCKET s = INVALID_SOCKET;
    int TTL = DEFAULT_TTL;
    int packetlen = 0;
    char* icmpbuf = NULL;
    ICMP_HDR* icmp_hdr = NULL;
    char* datapart = NULL;
    WSAOVERLAPPED recvol;
    recvol.hEvent = WSA_INVALID_EVENT;
    SOCKADDR_STORAGE from;
    int fromlen = sizeof(SOCKADDR_STORAGE);
    DWORD flags, bytes;
    char recvbuf[MAX_RECV_BUF_LEN];
    int recvbuflen = MAX_RECV_BUF_LEN;
    ULONG time = 0;
    UINT RecvPack = 0;

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " URL " << endl;
        return -1;
    }
    destHost = argv[1];

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("WSAStartup failed with error: %d\n", iResult);
    }

    remote = ResolveAddress(destHost, AF_UNSPEC, 0, 0);
    local = ResolveAddress(NULL, remote->ai_family, 0, 0);

    //---------------------------------------------
    // Create a socket for sending data
    s = socket(remote->ai_family, SOCK_RAW, IPPROTO_ICMP);
    if (s == INVALID_SOCKET) {
        printf("socket() failed with %d\n",WSAGetLastError());
        freeaddrinfo(remote);
        freeaddrinfo(local);
        WSACleanup();
        return -1;
    }

    iResult = setsockopt(s, IPPROTO_IP, IP_TTL, (char*)&TTL, sizeof(int));
    if (iResult == SOCKET_ERROR) {
        printf("setsockopt failed with %d\n",GetLastError());
        freeaddrinfo(remote);
        freeaddrinfo(local);
        closesocket(s);
        WSACleanup();
        return -1;
    }

    //---------------------------------------------
    //Construct an ICMP message;
    packetlen += sizeof(ICMP_HDR);
    packetlen += DEFAULT_SIZE;
    icmpbuf = (char*)malloc(packetlen);
    if (icmpbuf == NULL) {
        printf("malloc failed with %d\n",GetLastError());
        freeaddrinfo(remote);
        freeaddrinfo(local);
        closesocket(s);
        WSACleanup();
        return -1;
    }
    memset(icmpbuf, 0, packetlen);

    icmp_hdr = (ICMP_HDR*)icmpbuf;
    icmp_hdr->icmp_type = 8;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = (unsigned short)GetCurrentProcessId();
    icmp_hdr->icmp_sequence = 0;
    icmp_hdr->icmp_checksum = 0;

    datapart = icmpbuf + sizeof(ICMP_HDR);
    memset(datapart, 'Q', DEFAULT_SIZE);

    //---------------------------------------------
    //Bind socket;
    iResult = bind(s, local->ai_addr, (int)local->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with %d\n",WSAGetLastError());
        freeaddrinfo(remote);
        freeaddrinfo(local);
        closesocket(s);
        free(icmpbuf);
        WSACleanup();
        return -1;
    }
   
    //---------------------------------------------
    //Create wait event;
    memset(&recvol, 0, sizeof(WSAOVERLAPPED));
    recvol.hEvent = WSACreateEvent();
    if (recvol.hEvent == WSA_INVALID_EVENT) {
        printf("WSACreateEvent faild with %d\n",WSAGetLastError());
        freeaddrinfo(remote);
        freeaddrinfo(local);
        closesocket(s);
        free(icmpbuf);
        WSACleanup();
        return -1;
    }

    //---------------------------------------------
    //Receive information
    RecvFrom(s, recvbuf, recvbuflen, (SOCKADDR*)&from, &fromlen, &recvol);

    printf("Pinging: %s",destHost);
    PrintAddress(remote->ai_addr, remote->ai_addrlen);
    printf(" with %d bytes of data.\n",DEFAULT_SIZE);

    //---------------------------------------------
    //start ping
    for (int i = 0; i < 4; i++) {
        SetIcmpSequence(icmpbuf);
        ComputeIcmpchecksum(icmpbuf, packetlen);

        time = (ULONG)GetTickCount64();
        //Use “sendto” to send the ICMP message to the remote machine
        iResult = sendto(s, icmpbuf, packetlen, 0, remote->ai_addr, (int)remote->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("sendto failed with %d\n",WSAGetLastError());
            freeaddrinfo(remote);
            freeaddrinfo(local);
            closesocket(s);
            free(icmpbuf);
            WSACloseEvent(recvol.hEvent);
            WSACleanup();
            return -1;
        }

        iResult = WaitForSingleObject((HANDLE)recvol.hEvent, DEFAULT_RECV_TIMEOUT);
        if (iResult == WAIT_FAILED) {
            printf("WaitForSigleObject failed with %d\n",WSAGetLastError());
            freeaddrinfo(remote);
            freeaddrinfo(local);
            closesocket(s);
            free(icmpbuf);
            WSACloseEvent(recvol.hEvent);
            WSACleanup();
            return -1;
        }
        //runtime error
        else if (iResult == WAIT_TIMEOUT) {
            printf("Request Time Out.\n");
        }
        else {
            time = (ULONG)GetTickCount64() - time;
            WSAResetEvent(recvol.hEvent);
            RecvPack += 1;

            printf("Reply From");
            PrintAddress((SOCKADDR*)&from, fromlen);
            if (time == 0) {
                printf(": bytes = %d time < 1 ms TTL = %d\n", DEFAULT_SIZE, TTL);
            }
            else {
                printf(": bytes = %d time = %d ms TTL = %d\n", DEFAULT_SIZE, time, TTL);
            }
            if (i < 3) {
                fromlen = sizeof(SOCKADDR_STORAGE);
                RecvFrom(s, recvbuf, recvbuflen, (SOCKADDR*)&from, &fromlen, &recvol);
            }
        }
        Sleep(1000);

    }
    printf("Send 4 packages,  Receive %d packages,  Loss %d(%0.2lf%%) packages.\n",
        RecvPack, 4 - RecvPack,
        (4 - RecvPack) * 100.0 / 4);
    WSACleanup();
    return 0;
}

//---------------------------------------------
//checksum function
//input buffer, size, output checksum
//Step1: add 16 - bit integers using regular 32 - bit addition
//Step2 : take the 16 higher - order bits in the sum, shifting them down by 16 positions, and add them back to the sum
//Step3 : take 1’s complement of the sum(convert 1 to 0 and vice versa)

USHORT checksum(USHORT* buffer, int size) {
    unsigned long cksum = 0;
    while (size > 1) {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size) {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (USHORT)(~cksum);
}
//---------------------------------------------
//set checksum value
void ComputeIcmpchecksum(char* buf, int packetlen) {
    ICMP_HDR* icmpv4 = NULL;
    icmpv4 = (ICMP_HDR*)buf;
    icmpv4->icmp_checksum = 0;
    icmpv4->icmp_checksum = checksum((USHORT*)buf, packetlen);
}
//---------------------------------------------
//set sequence number
void SetIcmpSequence(char* buf) {
    ULONG sequence = 0;
    sequence = (ULONG)GetTickCount64();
    ICMP_HDR* icmpv4 = NULL;
    icmpv4 = (ICMP_HDR*)buf;
    icmpv4->icmp_sequence = (USHORT)sequence;
}

//---------------------------------------------
//given certain sockaddr point, print IPv4 address
int	PrintAddress(struct sockaddr* addr, int addrlen) {
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int	hostlen = NI_MAXHOST;
    int servlen = NI_MAXSERV;
    int iResult;

    iResult = getnameinfo(addr, addrlen, host, hostlen, serv, servlen, NI_NUMERICHOST | NI_NUMERICSERV);
    if (iResult != 0) {
        printf("getnameinfo failed: %d\n",iResult);
        return SOCKET_ERROR;
    }
    if (strcmp(serv, "0") != 0) {
        printf(" [%s:%s]", host, serv);
    }
    else {
        printf(" [%s]", host);
    }
    return NO_ERROR;
}
//---------------------------------------------
//input is addr(url), output is addrinfo
struct addrinfo* ResolveAddress(char* addr, int af, int type, int proto) {
    struct addrinfo hints, * res = NULL;
    int iResult;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = ((addr) ? 0 : AI_PASSIVE);
    hints.ai_family = af;
    hints.ai_socktype = type;
    hints.ai_protocol = proto;

    iResult = getaddrinfo(addr, DEFAULT_PORT, &hints, &res);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return NULL;
    }
    return res;
}

//---------------------------------------------
//receive datagram
//The WSARecvFrom function receives a datagram and stores the source address.
//The WSARecvFrom function provides functionality overand above the standard recvfrom function
int RecvFrom(SOCKET s, char* buf, int buflen, SOCKADDR* from, int* fromlen, WSAOVERLAPPED* ol) {
    WSABUF wbuf;
    DWORD flags, bytes;
    int iResult;
    wbuf.buf = buf;
    wbuf.len = buflen;
    flags = NULL;

    iResult = WSARecvFrom(s, &wbuf, 1, &bytes, &flags, from, fromlen, ol, NULL);
    if (iResult == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            printf("WSARecvfrom failed: %d\n", WSAGetLastError());
            return SOCKET_ERROR;
        }
    }
    return NO_ERROR;
}