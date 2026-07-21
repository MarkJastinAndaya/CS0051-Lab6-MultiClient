// ============================================================
// Server.cpp
// CS0051 - Parallel and Distributed Computing
// Exercise 6: Multi-Client TCP Server (Winsock2, C++17)
//
// This server accepts multiple simultaneous TCP client
// connections. Each client connection is handled on its own
// detached std::thread so the main thread can immediately
// continue accepting new incoming connections (asynchronous
// accept loop).
//
// Protocol:
//   Client sends:  "10 20"   (two integers separated by a space)
//   Server replies: "30"     (their sum, as plain text)
//
// Compile (MinGW / g++ on Windows):
//   g++ Server.cpp -o Server.exe -lws2_32 -pthread
// ============================================================

#include <winsock2.h>   // Core Winsock API (sockets, bind, listen, accept, send, recv)
#include <ws2tcpip.h>   // Extended TCP/IP functions (not strictly required here, but standard include)
#include <thread>       // std::thread for handling each client concurrently
#include <string>       // std::string for building/parsing messages
#include <sstream>      // std::stringstream for parsing "int int" from client text
#include <iostream>     // std::cout / std::cerr for console logging
#include <vector>       // std::vector (kept available per requirements, e.g. for future extension)

// Link the Winsock library automatically when using MSVC.
// (Harmless / ignored by g++; g++ users must link -lws2_32 manually on the command line.)
#pragma comment(lib, "ws2_32.lib")

// ------------------------------------------------------------
// Global configuration
// ------------------------------------------------------------
const int SERVER_PORT = 8080;   // Port the server listens on
const int BACKLOG     = SOMAXCONN; // Max pending connection queue length

// ------------------------------------------------------------
// Global shared state
// ------------------------------------------------------------
// The listening socket is shared between the setup code and the
// accept loop, so it is declared globally as required by the
// lab's asynchronous multi-client design.
SOCKET g_serverSocket = INVALID_SOCKET;

// ------------------------------------------------------------
// initializeWinsock()
// ------------------------------------------------------------
// Starts up the Winsock library (WSAStartup). This MUST be called
// before any socket function is used on Windows.
// Returns true on success, false on failure.
// ------------------------------------------------------------
bool initializeWinsock()
{
    WSADATA wsaData;

    // Request Winsock version 2.2
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return false;
    }

    std::cout << "Winsock initialized successfully." << std::endl;
    return true;
}

// ------------------------------------------------------------
// createServerSocket()
// ------------------------------------------------------------
// Creates a TCP socket, binds it to SERVER_PORT on all local
// interfaces (INADDR_ANY), and puts it into listening mode.
// Stores the resulting socket in the global g_serverSocket.
// Returns true on success, false on failure.
// ------------------------------------------------------------
bool createServerSocket()
{
    // Step 1: Create a TCP socket (IPv4, stream-based, TCP protocol)
    g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_serverSocket == INVALID_SOCKET)
    {
        std::cerr << "socket() failed with error: " << WSAGetLastError() << std::endl;
        return false;
    }

    BOOL opt = TRUE;
    setsockopt(
        g_serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char*)&opt,
        sizeof(opt)
    );

    // Step 2: Prepare the sockaddr_in structure describing the
    // local address/port to bind to.
    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;         // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;      // Accept connections on any local interface
    serverAddr.sin_port        = htons(SERVER_PORT); // Convert port to network byte order

    // Step 3: Bind the socket to the address/port
    if (bind(g_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
        closesocket(g_serverSocket);
        return false;
    }

    // Step 4: Put the socket into listening mode so it can accept
    // incoming connection requests.
    if (listen(g_serverSocket, BACKLOG) == SOCKET_ERROR)
    {
        std::cerr << "listen() failed with error: " << WSAGetLastError() << std::endl;
        closesocket(g_serverSocket);
        return false;
    }

    std::cout << "==================================" << std::endl;
    std::cout << "Server Started" << std::endl;
    std::cout << "Listening on port " << SERVER_PORT << "..." << std::endl;
    std::cout << "==================================" << std::endl;

    return true;
}

// ------------------------------------------------------------
// handleClient(SOCKET clientSocket)
// ------------------------------------------------------------
// Runs on its own detached thread for exactly one connected
// client. It:
//   1. Receives a text message "int int"
//   2. Parses the two integers using std::stringstream
//   3. Computes their sum
//   4. Sends the sum back as plain text
//   5. Closes the client socket
// ------------------------------------------------------------
void handleClient(SOCKET clientSocket)
{
    std::cout << "\n========================================\n";
    std::cout << " Client Connected\n";
    std::cout << "========================================\n";

    char recvBuffer[1024] = {};

    int bytesReceived = recv(
        clientSocket,
        recvBuffer,
        sizeof(recvBuffer) - 1,
        0
    );

    if (bytesReceived > 0)
    {
        recvBuffer[bytesReceived] = '\0';

        std::string receivedText(recvBuffer);

        std::stringstream ss(receivedText);

        int num1, num2;
        ss >> num1 >> num2;

        int sum = num1 + num2;

        std::string response = std::to_string(sum);

        send(
            clientSocket,
            response.c_str(),
            (int)response.length(),
            0
        );

        std::cout << "\n";
        std::cout << "Received      : " << receivedText << "\n";
        std::cout << "Computed Sum  : " << sum << "\n";
        std::cout << "Response Sent : " << response << "\n";
    }
    else if (bytesReceived == 0)
    {
        std::cout << "\n";
        std::cout << "Status        : Client disconnected before sending data.\n";
    }
    else
    {
        std::cerr << "\n";
        std::cerr << "Receive Error : " << WSAGetLastError() << "\n";
    }

    closesocket(clientSocket);

    std::cout << "\n";
    std::cout << "Client Disconnected\n";
    std::cout << "----------------------------------------\n";
}

// ------------------------------------------------------------
// acceptClients()
// ------------------------------------------------------------
// Runs forever, continuously accepting new incoming client
// connections on g_serverSocket. For every accepted client, a
// brand-new std::thread running handleClient() is spawned and
// immediately detached, so that:
//   - the client is served independently and concurrently
//   - the main accept loop is never blocked while a client is
//     being served (true multi-client / asynchronous behavior)
// ------------------------------------------------------------
void acceptClients()
{
    while (true)
    {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        // Blocks here until a new client connects. Once a client
        // connects, control returns immediately with a socket
        // dedicated to that client.
        SOCKET clientSocket = accept(g_serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
            // Continue the loop; one failed accept shouldn't kill the server.
            continue;
        }

        // Spawn a dedicated thread for this client so the loop can
        // immediately go back to accept() and wait for the next
        // client without waiting for this one to finish.
        std::thread clientThread(handleClient, clientSocket);

        // Detach the thread: it will run independently and clean
        // up its own resources (closing the socket) when done.
        clientThread.detach();
    }
}

// ------------------------------------------------------------
// cleanup()
// ------------------------------------------------------------
// Closes the listening socket and shuts down Winsock. In this
// design the server is meant to run forever, so this function
// is provided for completeness (e.g., if a shutdown path were
// ever added) and to satisfy the required code organization.
// ------------------------------------------------------------
void cleanup()
{
    if (g_serverSocket != INVALID_SOCKET)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
    std::cout << "Server resources cleaned up." << std::endl;
}

// ------------------------------------------------------------
// main()
// ------------------------------------------------------------
// Coordinates the overall server lifecycle:
//   1. initializeWinsock()
//   2. createServerSocket()
//   3. acceptClients()  -- runs forever, spawning a thread per client
//   4. cleanup()        -- only reached if acceptClients() ever returns
// ------------------------------------------------------------
int main()
{
    // Step 1: Start up Winsock
    if (!initializeWinsock())
    {
        return 1;
    }

    // Step 2: Create, bind, and listen on the server socket
    if (!createServerSocket())
    {
        WSACleanup();
        return 1;
    }

    // Step 3: Continuously accept and serve clients.
    // Each client is handled on its own detached thread, so this
    // call effectively never returns during normal operation.
    acceptClients();

    // Step 4: Cleanup (only reached if acceptClients() somehow exits)
    cleanup();

    return 0;
}
