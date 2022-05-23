//#include "pch.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#define NI_MAXSERV    32
#define NI_MAXHOST  1025


#pragma comment(lib,"Ws2_32.lib")// Used to link with Ws2_32.lib

#include <iostream>
using namespace std;

int gethostname(
    _Out_ char* name, //A pointer to a buffer that receives the local host name.
    _In_  int  namelen  //The length, in bytes, of the buffer pointed to by the name parameter.
);


int main(int argc, char* argv[]) {
    WORD wVersion = MAKEWORD(2, 2); // Used to request version 2.2 of Windows sockets
    WSADATA wsaData;                // Data loaded by WSAStartup
    int iResult;                    // Error check if WSAStartup successful
	const char* localhost = "127.0.0.1";
	struct sockaddr_in saGHN;
	char hostname[NI_MAXHOST];
	char servInfo[NI_MAXSERV];
	u_short port = 27015;

    // Initialize Winsock
    iResult = WSAStartup(wVersion, &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1;
    }
    
	if (argc == 1) {
			inet_pton(AF_INET, localhost, &saGHN.sin_addr.s_addr);
		}
	else {
		inet_pton(AF_INET, argv[2], &saGHN.sin_addr.s_addr);
	}

	DWORD dwRetval;



	//-----------------------------------------
	// Set up sockaddr_in structure which is passed
	// to the getnameinfo function
	saGHN.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &saGHN.sin_addr.s_addr);
	saGHN.sin_port = htons(port);

	//-----------------------------------------
	// Call getnameinfo
	dwRetval = getnameinfo((struct sockaddr*)&saGHN,
		sizeof(struct sockaddr),
		hostname,
		NI_MAXHOST, servInfo, NI_MAXSERV, 0);

	if (dwRetval != 0) {
		printf("getnameinfo failed with error # %ld\n", WSAGetLastError());
		return 1;
	}
	else {
		printf("getnameinfo returned hostname = %s\n", hostname);
		return 0;
	}

	WSACleanup();
    return 0;
}
