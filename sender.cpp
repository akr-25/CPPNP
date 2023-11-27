#include <iostream>
#include <cstring>
#include <thread>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include "streaminfo.cpp"
#include <fstream>

#define MULTICAST_IP "239.0.0.1"
#define PORT 8888
#define BUFFER_SIZE 32000

StreamInfo stream("0.0.0.0", 0, "video1", "videoDescription", MULTICAST_IP, PORT);

void handlerTracker(const std::string& ip_address, const int port)
{
  int sockfd;
  struct sockaddr_in tracker_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Failed to create socket");
  }

  // Set up the tracker address
  memset(&tracker_addr, 0, sizeof(tracker_addr));
  tracker_addr.sin_family = AF_INET;
  tracker_addr.sin_port = htons(port);
  tracker_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());

  // Connect to the tracker
  if (connect(sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0)
  {
    perror("Failed to connect to tracker");
  }

  // Send the existence message
  std::string message = stream.encode() + "\n";
  if (send(sockfd, message.c_str(), message.length(), 0) < 0)
  {
    perror("Failed to send message");
  }

  // sendHeartbeat
  while (true)
  {
    std::string heartbeat = "heartbeat\n";
    std::cout << "Sending heartbeat to " << ip_address << ":" << port << std::endl;
    if (send(sockfd, heartbeat.c_str(), heartbeat.length(), 0) < 0)
    {
      perror("Failed to send heartbeat");
    }
    sleep(2);
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <ip_address> <port>" << std::endl;
    return -1;
  }

  std::string ip_address = argv[1];
  int port = atoi(argv[2]);

  std::thread trackerThread(handlerTracker, ip_address, port);

  int sockfd;
  struct sockaddr_in multicast_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    perror("Failed to create socket");
    return -1;
  }

  // Set the multicast TTL
  unsigned char multicastTTL = 1; // Set according to your network's requirement
  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &multicastTTL, sizeof(multicastTTL)) < 0)
  {
    std::cerr << "Error setting multicast TTL" << std::endl;
    return 1;
  }

  // Set up the multicast destination address
  memset(&multicast_addr, 0, sizeof(multicast_addr));
  multicast_addr.sin_family = AF_INET;
  multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
  multicast_addr.sin_port = htons(PORT);

  std::ifstream file("Up.mp4", std::ios::binary | std::ios::ate);
  if (!file.is_open())
  {
    std::cerr << "Failed to open file" << std::endl;
    return -1;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  char *buffer = new char[BUFFER_SIZE];
  
  while (!file.eof())
  {
    file.read(buffer, BUFFER_SIZE);
    if (sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
    {
      perror("Failed to send message");
      close(sockfd);
      return -1;
    }
    // std::cout << "Message sent to multicast group." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Wait for 2 seconds before sending the next message
  }

  file.close();

  // // Sender loop
  // while (true)
  // {
  //   const char *message = "Hello, multicast group!";
  //   if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
  //   {
  //     perror("Failed to send message");
  //     close(sockfd);
  //     return -1;
  //   }

  //   std::cout << "Message sent to multicast group." << std::endl;
  //   sleep(2); // Wait for 2 seconds before sending the next message
  // }

  close(sockfd);
  return 0;
}
