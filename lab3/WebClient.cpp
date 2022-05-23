#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")// Used to link with Ws2_32.lib
#include <iostream>
using namespace std;

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 512

int main(int argc, char* argv[]) {
	WORD wVersion = MAKEWORD(2, 2); // Used to request version 2.2 of Windows sockets
	WSADATA wsaData;                // Data loaded by WSAStartup
	int iResult;                    // Error check if WSAStartup successful
	u_short port = 27015;
	const char* sendbuf = "GET / HTTP/1.0\n\n";

	//1
	//Declare an addrinfo object
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//2
	// Resolve the server address and port
	iResult = WSAStartup(wVersion, &wsaData);
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed: " << iResult << endl;
		WSACleanup();
		return 1;
	}

	//3
	//Create a SOCKET object called ClientSocket.
	SOCKET ClientSocket = INVALID_SOCKET;

	//4
	// Attempt to connect to the first address returned by
    // the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ClientSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	//5
	// Connect to server.
	iResult = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ClientSocket);
		ClientSocket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (ClientSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!\n";
		WSACleanup();
		return 1;
	}

	//6
	// Send an initial buffer
	iResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		cout << "send failed: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	cout << "Bytes Sent: " << iResult << endl;

	//7
	// shutdown the connection for sending since no more data will be sent
    // the client can still use the ClientSocket for receiving data
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	//8
	// Receive data until the server closes the connection
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			cout << "Bytes received: " << iResult << endl;
			cout << recvbuf << endl;
		}
		else if (iResult == 0)
			cout << "Connection closed\n";
		else
			cout << "recv failed: " << WSAGetLastError() << endl;
	} while (iResult > 0);

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}
