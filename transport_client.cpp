#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "transport_enums.hpp"

class TransportClient
{
public:
  using ConnectCallback = std::function<void()>;
  using DisconnectCallback = std::function<void()>;
  using DataCallback = std::function<void(const std::string &)>;

  TransportClient(ConnectionType type, ConnectionMode mode, Role role);

  void RegisterOnConnectCallback(ConnectCallback callback);
  void RegisterOnDisconnectCallback(DisconnectCallback callback);
  void RegisterOnDataCallback(DataCallback callback);

  void Connect(const std::string &address, uint16_t port);
  void Disconnect();
  void Send(const std::string &message);

private:
  ConnectionType type_;
  ConnectionMode mode_;
  Role role_;
  std::string address_;
  int socket_;
  int port_;

  std::vector<ConnectCallback> connectCallbacks_;
  std::vector<DisconnectCallback> disconnectCallbacks_;
  std::vector<DataCallback> dataCallbacks_;

  // Add socket-related members here
};

TransportClient::TransportClient(ConnectionType type, ConnectionMode mode, Role role)
    : type_(type), mode_(mode), role_(role), socket_(-1)
{
  // Initialize socket-related code here based on type, mode, and role
}

void TransportClient::RegisterOnConnectCallback(ConnectCallback callback)
{
  connectCallbacks_.push_back(callback);
}

void TransportClient::RegisterOnDisconnectCallback(DisconnectCallback callback)
{
  disconnectCallbacks_.push_back(callback);
}

void TransportClient::RegisterOnDataCallback(DataCallback callback)
{
  dataCallbacks_.push_back(callback);
}

void TransportClient::Connect(const std::string &address, uint16_t port)
{
  if (socket_ != -1)
  {
    perror("Socket already initialised!");
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
  inet_pton(AF_INET, address.c_str(), &serverAddress.sin_addr);
  // serverAddress.sin_addr.s_addr = inet_addr(address.c_str());

  while (true)
  {
    // Connect to the server
    if (connect(socket_, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
      if (errno == ECONNREFUSED)
      {
        printf("Server not Online\n");
        usleep(5000000);
        continue;
      }
      close(socket_);
      socket_ = -1;
      return;
    }
    else{ /*---- Successful Connection ----*/
      break;
    }
  }
  for (const auto &callback : connectCallbacks_)
  {
    callback();
  }
}

void TransportClient::Disconnect()
{
  if (socket_ != -1)
  {
    close(socket_);
    socket_ = -1;
    for (const auto &callback : disconnectCallbacks_)
    {
      callback();
    }
  }
  /*--------NOT A GRACEFUL WAY TO DO--------*/
  exit(EXIT_FAILURE);  
}

void TransportClient::Send(const std::string &message)
{
  if (socket_ == -1)
  {
    perror("Socket not created yet!");
    return;
  }

  // Send data
  int bytesSent = send(socket_, message.c_str(), message.length(), 0);
  if (bytesSent == -1)
  {
    perror("Send failed");
    Disconnect();
    return;
  }
}
