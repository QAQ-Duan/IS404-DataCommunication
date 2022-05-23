
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>         // Needed for memcpy() and strcpy()
#include <process.h>        // Needed for _beginthread() and _endthread()
#include <fcntl.h>          // For binary handle options
#include <sys\stat.h>     // For binary write()
#include <io.h>             // Needed for open(), close(), write()

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")


//----- HTTP response messages ----------------------------------------------
#define OK_IMAGE  "HTTP/1.0 200 OK\r\nContent-Type:image/gif\r\n\r\n"
#define OK_TEXT   "HTTP/1.0 200 OK\r\nContent-Type:text/html\r\n\r\n"
#define NOTOK_404 "HTTP/1.0 404 Not Found\r\nContent-Type:text/html\r\n\r\n"
#define MESS_404  "<html><body><h1>FILE NOT FOUND</h1></body></html>"

//----- Defines -------------------------------------------------------------
#define  BUF_SIZE            1024     // Buffer size (big enough for a GET)
#define  PORT_NUM              80     // Port number for a Web server

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "80"

//----- Globals ---------------------------------------------------------------
int      Count;                 // Thread counter         
//----- Function prototypes -------------------------------------------------
//===========================================================================
//=  Thread function to service a single client                             =
//===========================================================================
void do_service(void* client_s)
{
    char                 in_buf[1024];     // Input buffer for response

    printf("thread beninging... \n");

    // Loop until client shut down
    while (1)
    {
        // Receive from the client
        if (recv((unsigned int)client_s, in_buf, sizeof(in_buf), 0) == 0)
            break; // when client shut down
        printf("Received from client... data = '%s' \n", in_buf);

        // Echo the received message to the client
        send((unsigned int)client_s, in_buf, (strlen(in_buf) + 1), 0);
    }

    printf("thread completed... \n");
    // Decrement for a completed thread
    Count--;

    // Close all open sockets and end the thread
    closesocket((unsigned int)client_s);

    _endthread();
}
//===========================================================================
//=  This is the thread function to handle the GET                       =
//===========================================================================
void handle_get(void* in_arg)
{
    unsigned int   client_s;             // Client socket descriptor
    char           in_buf[BUF_SIZE];     // Input buffer for GET request
    char           out_buf[BUF_SIZE];    // Output buffer for HTML response
    int            fh;                   // File handle
    int            buf_len;              // Buffer length for file reads
    char           command[BUF_SIZE];    // Command buffer
    char           file_name[BUF_SIZE];  // File name buffer
    int            retcode;              // Return code
    int			 j;

    // Set client_s to in_arg
    client_s = (unsigned int)in_arg;

    printf("thread %d... \n", client_s);
    // Receive the (presumed) GET request from the Web browser
    retcode = recv(client_s, in_buf, BUF_SIZE, 0);
    printf("thread %d...received web request: \n", client_s);
    for (j = 0; j < retcode; j++)
        printf("%c", in_buf[j]);

    // If the recv() return code is bad then bail-out (see note #3)
    if (retcode <= 0)
    {
        printf("ERROR - Receive failed --- probably due to dropped connection \n");
        closesocket(client_s);
        _endthread();
    }

    // Parse out the command from the (presumed) GET request and filename
    sscanf_s(in_buf, "%s %s \n", &command, BUF_SIZE, &file_name, BUF_SIZE);

    // Check if command really is a GET, if not then bail-out
    if (strcmp(command, "GET") != 0)
    {
        printf("ERROR - Not a GET --- received command = '%s' \n", command);
        closesocket(client_s);
        _endthread();
    }

    // It must be a GET... open the requested file
    //  - Start at 2nd char to get rid of leading "\"
    _sopen_s(&fh, &file_name[1], _O_RDONLY | _O_BINARY,
        _SH_DENYNO, _S_IREAD | _S_IWRITE);

    // If file does not exist, then return a 404 and bail-out
    if (fh == -1)
    {
        printf("File '%s' not found --- sending an HTTP 404 \n", &file_name[1]);
        strcpy_s(out_buf, NOTOK_404);
        send(client_s, out_buf, strlen(out_buf), 0);
        strcpy_s(out_buf, MESS_404);
        send(client_s, out_buf, strlen(out_buf), 0);
        closesocket(client_s);
        _endthread();
    }

    // Check that filename does not start with a "..", "/", "\", or have a ":" in
    // the second position indicating a disk identifier (e.g., "c:").
    //  - This is a security check to prevent grabbing any file on the server
    if (((file_name[1] == '.') && (file_name[2] == '.')) ||
        (file_name[1] == '/') || (file_name[1] == '\\') ||
        (file_name[2] == ':'))
    {
        printf("SECURITY VIOLATION --- trying to read '%s' \n", &file_name[1]);
        _close(fh);
        closesocket(client_s);
        _endthread();
    }

    // Generate and send the response
    printf("Thread %d, ...Sending file '%s' \n", in_arg, &file_name[1]);
    //search .gif in file_name
    if (strstr(file_name, ".gif") != NULL)
        strcpy_s(out_buf, OK_IMAGE);
    else
        strcpy_s(out_buf, OK_TEXT);
    send(client_s, out_buf, strlen(out_buf), 0);
    while (!_eof(fh))
    {
        buf_len = _read(fh, out_buf, BUF_SIZE);
        send(client_s, out_buf, buf_len, 0);
    }

    // Close the file, close the client socket, and end the thread
    _close(fh);
    printf("Thread %d, ...comleted sending file '%s' \n", client_s, &file_name[1]);
    closesocket(client_s);
    printf("socket %d closed. \n", client_s);
    printf("thread %d ended. \n", client_s);
    _endthread();
}



int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    unsigned int         client_s;        // Client socket descriptor
    struct sockaddr_in   client_addr;     // Client Internet address
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Internet address length
    char ipstringbuffer[46];


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }


    // Main loop to listen, accept, and then spin-off a thread to handle the GET
    while (1)
    {
        printf("main loop: linstening ... \n");
        // Listen for connections and then accept
        listen(ListenSocket, 50);
        addr_len = sizeof(client_addr);
        client_s = accept(ListenSocket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_s == -1)
        {
            printf("ERROR - Unable to create a socket \n");
            exit(1);
        }
        printf("client socket accepted, %d... \n", client_s);
        // Spin-off a thread to handle this request (pass only client_s)
        if (_beginthread(handle_get, 4096, (void*)client_s) < 0)
        {
            printf("ERROR - Unable to create a thread to handle the GET \n");
            exit(1);
        }
    }

    printf("main loop completed. close server socket... WSAcleanup \n");
    // Close the server socket and clean-up winsock
    closesocket(ListenSocket);
    WSACleanup();
    /*
    // Main loop (Loop forever)
    Count = 0;
    while (1)
    {
        Count++;
        printf("Count=%d \n", Count);
        // Accept a connection.  The accept() will block and then return with
        // client_addr filled-in.
        addr_len = sizeof(client_addr);
        client_s = accept(ListenSocket, (struct sockaddr*)&client_addr, &addr_len);

        // Copy the four-byte client IP address into an IP address structure
        //   - See winsock.h for a description of struct in_addr
        memcpy(&client_ip_addr, &client_addr.sin_addr.s_addr, 4);

        // Print an informational message that accept completed
        printf("Connection %d accepted!!! \n", Count);
        inet_ntop(AF_INET, &client_ip_addr, ipstringbuffer, sizeof(ipstringbuffer));
        printf("\tClient socket number: %d\n", client_s);
        printf("\tIPv4 address: %s\n", ipstringbuffer);
        printf("\tPort nuber: %d\n", ntohs(client_addr.sin_port));

        if (_beginthread(do_service, 4096, (void*)client_s) < 0)
        {
            printf("ERROR - Unable to create thread \n");
            exit(1);
        }
    }

    // Never reached!!!
    // Wait for all threads to finish
    while (Count);

    // Close open sockets
    closesocket(ListenSocket);

    // This stuff cleans-up winsock
    WSACleanup();
    */
}
