#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "transport_enums.hpp"

class TransportServer {
public:
    using ConnectCallback = std::function<void()>;
    using DisconnectCallback = std::function<void()>;
    using DataCallback = std::function<void(const std::string&)>;

    TransportServer(ConnectionType type, ConnectionMode mode, Role role);

    void RegisterOnConnectCallback(ConnectCallback callback);
    void RegisterOnDisconnectCallback(DisconnectCallback callback);
    void RegisterOnDataCallback(DataCallback callback);

    void Listen(int port);
    void AcceptConnections();
    void Disconnect();
    void Send(const std::string& message);

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
};

TransportServer::TransportServer(ConnectionType type, ConnectionMode mode, Role role)
    : type_(type), mode_(mode), role_(role), socket_(-1), clientSocket_(-1) {
    // Initialize the socket-related code here based on type, mode, and role
}

void TransportServer::RegisterOnConnectCallback(ConnectCallback callback) {
    connectCallbacks_.push_back(callback);
}

void TransportServer::RegisterOnDisconnectCallback(DisconnectCallback callback) {
    disconnectCallbacks_.push_back(callback);
}

void TransportServer::RegisterOnDataCallback(DataCallback callback) {
    dataCallbacks_.push_back(callback);
}

void TransportServer::Listen(int port) {
    if (socket_ != -1) {
        // Already listening
        return;
    }

    // Create a socket (for TCP)
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind to the specified port
    if (bind(socket_, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Bind failed");
        close(socket_);
        socket_ = -1;
        return;
    }

    // Listen for incoming connections
    if (listen(socket_, 5) == -1) {
        perror("Listen failed");
        close(socket_);
        socket_ = -1;
        return;
    }
}

void TransportServer::AcceptConnections() {
    if (socket_ == -1) {
        // Not listening
        return;
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Accept incoming connections
    clientSocket_ = accept(socket_, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (clientSocket_ == -1) {
        perror("Accept failed");
        close(socket_);
        socket_ = -1;
        return;
    }

    for (const auto& callback : connectCallbacks_) {
        callback();
    }

    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(clientSocket_, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesRead] = '\0';
        for (const auto& callback : dataCallbacks_) {
            callback(buffer);
        }
    }

    Disconnect();
}

void TransportServer::Disconnect() {
    if (clientSocket_ != -1) {
        close(clientSocket_);
        clientSocket_ = -1;
        for (const auto& callback : disconnectCallbacks_) {
            callback();
        }
    }
    if (socket_ != -1) {
        close(socket_);
        socket_ = -1;
    }
}

void TransportServer::Send(const std::string& message) {
    if (clientSocket_ == -1) {
        // Not connected
        return;
    }

    // Send data
    int bytesSent = send(clientSocket_, message.c_str(), message.length(), 0);
    if (bytesSent == -1) {
        perror("Send failed");
        Disconnect();
    }
}