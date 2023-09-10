#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "transport_enums.hpp"

#include <thread>
#define ConnectionCount 10

class TransportServer
{
public:
    using ConnectCallback = std::function<void()>;
    using DisconnectCallback = std::function<void()>;
    using DataCallback = std::function<void(const std::string &)>;

    TransportServer(ConnectionType type, ConnectionMode mode, Role role);

    void RegisterOnConnectCallback(ConnectCallback callback);
    void RegisterOnDisconnectCallback(DisconnectCallback callback);
    void RegisterOnDataCallback(DataCallback callback);

    void Listen(int port);
    void AcceptConnections();
    void Disconnect();
    void Send(const std::string &message); 
    
    // static void HandleClient(int newClientSocket); //added
    // void Close(); //added


private:
    ConnectionType type_;
    ConnectionMode mode_;
    Role role_;
    int socket_;
    int clientSocket_;
    std::string address_;
    int port_;

    std::vector<ConnectCallback> connectCallbacks_;
    std::vector<DisconnectCallback> disconnectCallbacks_;
    std::vector<DataCallback> dataCallbacks_;
    std::vector<std::thread>clientThreads;
};

TransportServer::TransportServer(ConnectionType type, ConnectionMode mode, Role role)
    : type_(type), mode_(mode), role_(role), socket_(-1), clientSocket_(-1)
{
    // Initialize the socket-related code here based on type, mode, and role
}

void TransportServer::RegisterOnConnectCallback(ConnectCallback callback)
{
    connectCallbacks_.push_back(callback);
}

void TransportServer::RegisterOnDisconnectCallback(DisconnectCallback callback)
{
    disconnectCallbacks_.push_back(callback);
}

void TransportServer::RegisterOnDataCallback(DataCallback callback)
{
    dataCallbacks_.push_back(callback);
}

void TransportServer::Listen(int port)
{
    if (socket_ != -1)
    {
        perror("Server socket already initialised!");
        return;
    }

    // Create a socket (for TCP)
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1)
    {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind to the specified port
    if (bind(socket_, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Bind failed");
        close(socket_);
        socket_ = -1;
        return;
    }

    // Listen for incoming connections
    if (listen(socket_, 5) == -1)
    {
        perror("Listen failed");
        close(socket_);
        socket_ = -1;
        return;
    }
}

void TransportServer::AcceptConnections()
{
    if (socket_ == -1)
    {
        perror("Server socket not initialized!");
        return;
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    while (true)
    {
        // Accept incoming connections
        clientSocket_ = accept(socket_, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSocket_ == -1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN){
                // No pending connections, continue processing
                usleep(100000); // Sleep for 100 ms (adjust as needed)
                continue;
            }
            else{
                perror("Accept failed");
                close(socket_);
                socket_ = -1;
                return;
            }
        }

        for (const auto &callback : connectCallbacks_)
        {
            callback();
        }

        char buffer[1024];
        int bytesRead;

        while ((bytesRead = recv(clientSocket_, buffer, sizeof(buffer), 0)) > 0)
        {
            buffer[bytesRead] = '\0';
            for (const auto &callback : dataCallbacks_)
            {
                callback(buffer);
            }
        }
        

        // std::thread clientThread(HandleClient, clientSocket_);
        // clientThreads.push_back(std::move(clientThread));
        // clientThread.detach();


        Disconnect();
        /*---------Need to implement a way to close server------------*/
        // usleep(500000);
        // if(std::cin.peek()==EOF)
        //     continue;
        // std::string command;
        // getline(std::cin,command);
        // if(command=="close")
        // {
        //     Close();
        // }
    }
}

void TransportServer::Disconnect()
{
    if (clientSocket_ != -1)
    {
        close(clientSocket_);
        clientSocket_ = -1;
        for (const auto &callback : disconnectCallbacks_)
        {
            callback();
        }
    }
}

void TransportServer::Send(const std::string &message)
{
    if (clientSocket_ == -1)
    {
        perror("Client socket not initialized!");
        return;
    }

    // Send data
    int bytesSent = send(clientSocket_, message.c_str(), message.length(), 0);
    if (bytesSent == -1)
    {
        perror("Send() failed");
        Disconnect();
    }
}

/*------------NOT USED AS OF NOW------------
void TransportServer::HandleClient(int newClientSocket_)
{
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(newClientSocket_, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[bytesRead] = '\0';
        for (const auto &callback : dataCallbacks_)
        {
            callback(buffer);
        }
    }
}

void TransportServer::Close()
{
    if (clientSocket_ != -1)
    {
        close(clientSocket_);
    }
    if (socket_ != -1)
    {
        close(socket_);
    }
    printf("Stopping Server!");
    exit(EXIT_FAILURE);
}
*/