#include <iostream>
#include "transport.h"

int main()
{
  // Example usage of the Transport library
  TransportClient client(ConnectionType::TCP, ConnectionMode::Unicast, Role::Client);

  client.RegisterOnConnectCallback([]()
                                   { std::cout << "Connected to the server." << std::endl; });

  client.RegisterOnDisconnectCallback([]()
                                      { std::cout << "Disconnected from the server." << std::endl; });

  client.Connect("127.0.0.1", 12412);
  while(1)
  {
    std::string message;
    getline(std::cin,message);
    if(message=="close")
    {
      client.Disconnect();
      break;
    }
    client.Send(message);
  }

  // Implement the server-side code similarly
  while(true) {
    sleep(1);
  }

  return 0;
}