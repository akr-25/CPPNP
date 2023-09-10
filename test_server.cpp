#include <iostream>
#include "transport.h"


int main()
{
  // Example usage of the TransportServer library for a server
  TransportServer server(ConnectionType::TCP, ConnectionMode::Unicast, Role::Server);

  server.RegisterOnConnectCallback([]()
                                   { std::cout << "New client connected." << std::endl; });

  server.RegisterOnDisconnectCallback([]()
                                      { std::cout << "Client disconnected." << std::endl; });

  server.RegisterOnDataCallback([](const std::string &data)
                                { std::cout << "Received data: " << data << std::endl; });

  server.Listen(12412);
  server.AcceptConnections();

  return 0;
}
