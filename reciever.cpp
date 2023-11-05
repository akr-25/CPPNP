#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <gtk/gtk.h>
#include <thread>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <set>
#include <sstream>
#include <memory>

#include <mutex>


#include "streaminfo.cpp"

std::mutex streamsMutex;
std::vector<std::shared_ptr<StreamInfo>> streams; // List of all streams

void handleTracker(const std::string& ip_address, const int port){
  int sockfd;
  struct sockaddr_in server_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  // Set up the server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());

  // Connect to server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Failed to connect to tracker");
    exit(EXIT_FAILURE);
  }

  std::cout << "Connected to tracker" << std::endl;
  // print my port
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    perror("getsockname");
  else
    printf("My port number is %d\n", ntohs(sin.sin_port));

  // get the list of streams in a loop and update the list of streams

  while (true)
  {
    char buffer[10240];
    int bytesReceived = recv(sockfd, buffer, 10240, 0);
    if (bytesReceived < 0)
    {
      std::cout << "Trying again in 6 seconds, fucked" << std::endl;
      perror("Error receiving data from tracker");
      sleep(6);
      continue;
    }

    std::string data = std::string(buffer, bytesReceived);
    std::cout << "Received: " << data << std::endl;

    std::istringstream iss(data);
    std::string line;
    {
      std::lock_guard<std::mutex> lock(streamsMutex);
      streams.clear();
      while (std::getline(iss, line))
      {
        std::shared_ptr<StreamInfo> temp = std::make_shared<StreamInfo>(line);
        streams.push_back(std::move(temp));
      }
    }
    sleep(6);
  }
}

void on_show_button_clicked(GtkWidget *widget, gpointer user_data)
{
  StreamInfo *stream = (StreamInfo *)user_data;

  // For demonstration purposes, let's create a static message.
  std::string message = "Message from stream: " + stream->getName(); // Adjust as necessary

  // Create a new popup to show the message
  GtkWidget *message_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(message_popup), "Stream Message");
  gtk_window_set_default_size(GTK_WINDOW(message_popup), 250, 100);

  GtkWidget *message_label = gtk_label_new(message.c_str());
  gtk_container_add(GTK_CONTAINER(message_popup), message_label);

  gtk_widget_show_all(message_popup);
}

void on_row_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data)
{
  std::cout << "Row selected" << std::endl;
  // Fetch the StreamInfo object from the row's data
  StreamInfo *stream = (StreamInfo *)(user_data);
  if(stream == NULL) {
    std::cout << "Stream is null" << std::endl;
    return;
  }
  else {
    std::cout << "Stream is not null" << std::endl;
    std::cout << stream->encode() << std::endl;
    std::cout << "not null" << std::endl;
  }

  // Create a new popup window
  GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(popup), "Stream Details");
  gtk_window_set_default_size(GTK_WINDOW(popup), 250, 150);

  // Vertical box to contain the details and button
  GtkWidget *vbox_popup = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(popup), vbox_popup);


  // Create labels for the name and description
  GtkWidget *name_label = gtk_label_new(("Name: " + stream->getName()).c_str());
  GtkWidget *desc_label = gtk_label_new(("Description: " + stream->getDescription()).c_str()); // Assuming getDescription exists
  gtk_box_pack_start(GTK_BOX(vbox_popup), name_label, 0, 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_popup), desc_label, 0, 0, 0);

  // Create a "Show" button
  GtkWidget *show_button = gtk_button_new_with_label("Show");
  g_signal_connect(show_button, "clicked", G_CALLBACK(on_show_button_clicked), stream);
  gtk_box_pack_start(GTK_BOX(vbox_popup), show_button, 0, 0, 0);

  // Show the popup and its child widgets
  gtk_widget_show_all(popup);
}

void populateListbox(GtkWidget *listbox)
{
  GList *children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(listbox));
  for (iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);

  for (auto &stream : streams)
  {
    std::cout << "gtk --- " << stream->encode() << std::endl;
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(stream->getName().c_str()); // Assuming your StreamInfo has a toString method
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);

    // Connect the row to the "on_row_selected" callback
    g_signal_connect(row, "activate", G_CALLBACK(on_row_selected), stream.get());
    gtk_widget_show(row);
    gtk_widget_show(label);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
  }
}

gboolean updateListBox(gpointer user_data)
{
  GtkWidget *listbox = (GtkWidget *)user_data;
  populateListbox(listbox);
  return TRUE; // This ensures that the timeout function gets called repeatedly
}

void on_refresh_clicked(GtkWidget *widget, gpointer listbox)
{
  populateListbox(GTK_WIDGET(listbox));
}

void handleGTK(int argc, char *argv[]){
  gtk_init(&argc, &argv);
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Select Stream");
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget *listbox = gtk_list_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), listbox, 1, 1, 0);

  GtkWidget *refreshButton = gtk_button_new_with_label("Refresh");
  g_signal_connect(refreshButton, "clicked", G_CALLBACK(on_refresh_clicked), listbox);
  gtk_box_pack_start(GTK_BOX(vbox), refreshButton, 0, 0, 0);

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);
  updateListBox(listbox);
  g_timeout_add_seconds(6, updateListBox, listbox);
  gtk_main();
}

int main( int argc, char *argv[] )
{

  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <Tracker IP> <port>" << std::endl;
    return -1;
  }

  std::string ip_address = argv[1];
  int port = atoi(argv[2]);

  std::thread trackerThread(handleTracker, ip_address, port);
  std::thread gtkThread(handleGTK, argc, argv);

  trackerThread.join();
  gtkThread.join();
  return 0;
}
