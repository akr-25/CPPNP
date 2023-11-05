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

#define MULTICAST_IP "239.0.0.1"
#define PORT 8888

std::string videoName = "video1";
std::string videoDescription = "video1 description";

void handlerTracker(const std::string& ip_address, const int port)
{
  int sockfd;
  struct sockaddr_in tracker_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }

  // Send the existence message
  std::string message = videoName + " " + videoDescription;
  if (send(sockfd, message.c_str(), message.length(), 0) < 0)
  {
    perror("Failed to send message");
    exit(EXIT_FAILURE);
  }

  // sendHeartbeat
  while (true)
  {
    std::string heartbeat = "heartbeat";
    if (send(sockfd, heartbeat.c_str(), heartbeat.length(), 0) < 0)
    {
      perror("Failed to send heartbeat");
      exit(EXIT_FAILURE);
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

  // Set up the multicast destination address
  memset(&multicast_addr, 0, sizeof(multicast_addr));
  multicast_addr.sin_family = AF_INET;
  multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
  multicast_addr.sin_port = htons(PORT);

  // Sender loop
  while (true)
  {
    const char *message = "Hello, multicast group!";
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0)
    {
      perror("Failed to send message");
      close(sockfd);
      return -1;
    }

    std::cout << "Message sent to multicast group." << std::endl;
    sleep(2); // Wait for 2 seconds before sending the next message
  }

  close(sockfd);
  return 0;
}
