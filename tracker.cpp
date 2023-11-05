#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unordered_map>
#include <set>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <ctime>

#include "streaminfo.cpp"

#define masterClientPort 23008
#define masterServerPort 23007

std::vector<StreamInfo> streams; // List of all streams

void sendStreamersList(int clientSocket)
{
  while (true)
  {
    std::string clientList = "";
    for (auto &stream : streams)
    {
      if (stream.isAlive())
      {
        clientList += stream.encode();
        clientList += "\n";
      }
    }
    std::cout << "Sending streamers list to " << clientSocket << std::endl;

    ssize_t bytesSent = send(clientSocket, clientList.c_str(), clientList.length(), 0);
    if (bytesSent < 0)
    {
      perror("Error sending data to client");
      close(clientSocket); // Close the client socket
      return;              // Exit the function instead of terminating the entire program
    }
    sleep(3);
  }
}

void handleClients()
{
  int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (masterSocket < 0)
  {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  sockaddr_in masterAddr;
  masterAddr.sin_family = AF_INET;
  masterAddr.sin_port = htons(masterClientPort);
  masterAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(masterSocket, (sockaddr *)&masterAddr, sizeof(masterAddr)) < 0)
  {
    perror("Error binding socket");
    exit(EXIT_FAILURE);
  }

  if (listen(masterSocket, 5) < 0)
  {
    perror("Error listening on socket");
    exit(EXIT_FAILURE);
  }

  while (true)
  {
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSocket = accept(masterSocket, (sockaddr *)&clientAddr, &clientAddrSize);
    if (clientSocket < 0)
    {
      perror("Error accepting client");
      exit(EXIT_FAILURE);
    }

    std::thread clientThread(sendStreamersList, clientSocket);
    clientThread.detach();
  }
}

void handleServers()
{
  int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (masterSocket < 0)
  {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  sockaddr_in masterAddr;
  masterAddr.sin_family = AF_INET;
  masterAddr.sin_port = htons(masterServerPort);
  masterAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(masterSocket, (sockaddr *)&masterAddr, sizeof(masterAddr)) < 0)
  {
    perror("Error binding socket");
    exit(EXIT_FAILURE);
  }

  if (listen(masterSocket, 5) < 0)
  {
    perror("Error listening on socket");
    exit(EXIT_FAILURE);
  }

  while (true)
  {
    sockaddr_in serverAddr;
    socklen_t serverAddrSize = sizeof(serverAddr);
    int serverSocket = accept(masterSocket, (sockaddr *)&serverAddr, &serverAddrSize);
    if (serverSocket < 0)
    {
      perror("Error accepting server");
      exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int bytesReceived = recv(serverSocket, buffer, 1024, 0);
    if (bytesReceived < 0)
    {
      perror("Error receiving data from server");
      exit(EXIT_FAILURE);
    }

    std::string data = std::string(buffer, bytesReceived);

    std::string ipAddress = inet_ntoa(serverAddr.sin_addr);
    int port = ntohs(serverAddr.sin_port);

    // check for heartbeat
    if (data == "heartbeat")
    {
      std::cout << "Received heartbeat from " << ipAddress << ":" << port << std::endl;
      for (auto &stream : streams)
      {
        if (stream.getIpAddress() == ipAddress && stream.getPort() == port)
        {
          stream.resetHeartbeat();
          break;
        }
      }
      continue;
    }

    int divider = data.find_first_of(" ");
    std::string name = data.substr(0, divider);
    std::string description = data.substr(divider + 1);

    bool found = false;
    for (auto &stream : streams)
    {
      if (stream.getName() == name)
      {
        found = true;
        stream.setDescription(description);
        std::cout << "Updated stream " << name << " from " << ipAddress << ":" << port << std::endl;
        break;
      }
    }

    if (!found)
    {
      std::cout << "Added stream " << name << " from " << ipAddress << ":" << port << std::endl;
      streams.emplace_back(ipAddress, port, name, description);
    }
  }
}

int main()
{
  std::thread clientThread(handleClients);
  std::thread serverThread(handleServers);

  std::string input;
  std::cout << "Tracker is running" << std::endl;
  std::cout << "Type 'list' to list all streams" << std::endl;
  std::cout << "Type 'exit' to quit" << std::endl;
  while (true)
  {
    std::cin >> input;
    if (input == "exit")
    {
      break;
    }
    else if (input == "list")
    {
      std::cout << "List of all streams:" << std::endl;
      std::cout << "Name Description Viewers IP:Port Alive" << std::endl;
      for (auto &stream : streams)
      {
        std::cout << stream.getName() << " " << stream.getDescription() << " " << stream.getNumOfViewers() << " " << stream.getIpAddress() << ":" << stream.getPort() << std::boolalpha << stream.isAlive() << std::endl;
      }
    }
  }

  serverThread.detach();
  clientThread.detach();
  return 0;
}