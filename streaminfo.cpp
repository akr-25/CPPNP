#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <ctime>



class StreamInfo
{
private:
  std::string ipAddress;         // IP address of the sender
  int port;                      // Port number on which sender is broadcasting
  std::string name;              // Unique identifier for the stream/channel
  std::string description;       // Description or name for the stream
  std::set<std::string> viewers; // List of all viewers
  time_t lastHeartbeat;          // Last time the stream was updated

public:
  StreamInfo(const std::string &ip, int port, const std::string &name, const std::string &desc)
      : ipAddress(ip), port(port), name(name), description(desc), viewers({}), lastHeartbeat(time(NULL)) {}

  // Getters and Setters
  std::string getIpAddress() const { return ipAddress; }
  int getPort() const { return port; }
  std::string getName() const { return name; }
  std::string getDescription() const { return description; }
  int getNumOfViewers() const { return viewers.size(); }

  void setDescription(const std::string &desc) { description = desc; }
  void addViewer(const std::string &ip, int port)
  {
    viewers.insert(ip + ":" + std::to_string(port));
  }
  void removeViewer(const std::string &ip, int port)
  {
    viewers.erase(ip + ":" + std::to_string(port));
  }

  bool isAlive() const
  {
    return (time(NULL) - lastHeartbeat) < 10;
  }

  void resetHeartbeat()
  {
    lastHeartbeat = time(NULL);
  }

  StreamInfo(const std::string &encoded)
  {
    int pos = 0;
    int divider = encoded.find_first_of(" ", pos);
    ipAddress = encoded.substr(pos, divider - pos);
    pos = divider + 1;

    divider = encoded.find_first_of(" ", pos);
    port = std::stoi(encoded.substr(pos, divider - pos));
    pos = divider + 1;

    divider = encoded.find_first_of(" ", pos);
    name = encoded.substr(pos, divider - pos);
    pos = divider + 1;

    description = encoded.substr(pos);

    viewers = {};
    lastHeartbeat = time(NULL);

  }

  std::string encode() const
  {
    return ipAddress + " " + std::to_string(port) + " " + name + " " + description;
  }
};