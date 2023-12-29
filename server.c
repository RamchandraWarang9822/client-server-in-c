// server.c

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 2
#define MAX_BUFFER_SIZE 1024

typedef struct {
  int socket;
  char name[MAX_BUFFER_SIZE];
  struct sockaddr_in address;
} ClientInfo;

typedef struct {
  int num_clients;
  ClientInfo *clients[MAX_CLIENTS];
} ClientList;

void add_client(ClientList *client_list, ClientInfo *client) {
  if (client_list->num_clients < MAX_CLIENTS) {
    client_list->clients[client_list->num_clients++] = client;
  }
}

void remove_client(ClientList *client_list, ClientInfo *client) {
  int i;
  for (i = 0; i < client_list->num_clients; ++i) {
    if (client_list->clients[i] == client) {
      memmove(&client_list->clients[i], &client_list->clients[i + 1],
              (client_list->num_clients - i - 1) * sizeof(ClientInfo *));
      --client_list->num_clients;
      break;
    }
  }
}

void broadcast_message(ClientList *client_list, ClientInfo *sender,
                       const char *message) {
  for (int i = 0; i < client_list->num_clients; i++) {
    ClientInfo *receiver = client_list->clients[i];
    if (receiver->socket != sender->socket) {
      send(receiver->socket, message, strlen(message), 0);
    }
  }
}

void *handle_client(void *arg) {
  ClientList *client_list = (ClientList *)arg;
  ClientInfo *client_info = malloc(sizeof(ClientInfo));
  client_info->socket =
      client_list->clients[client_list->num_clients - 1]->socket;
  client_info->address =
      client_list->clients[client_list->num_clients - 1]->address;

  add_client(client_list, client_info);

  char buffer[MAX_BUFFER_SIZE] = {0};

  // Receive the username from the client
  int valread = read(client_info->socket, buffer, MAX_BUFFER_SIZE);
  strcpy(client_info->name, buffer);
  printf("New client connected with username %s on port %d\n",
         client_info->name, ntohs(client_info->address.sin_port));

  while (1) {
    memset(buffer, 0, sizeof(buffer));
    // Receive the message from the client
    valread = read(client_info->socket, buffer, MAX_BUFFER_SIZE);

    // Check if the client disconnected
    if (valread <= 0) {
      printf("%s on port %d disconnected\n", client_info->name,
             ntohs(client_info->address.sin_port));
      remove_client(client_list, client_info);
      break;
    }

    // Broadcast the message to all clients except the sender
    broadcast_message(client_list, client_info, buffer);

    // Print the message from the client
    printf("%s : %s\n", client_info->name, buffer);

    // Clear the buffer
    memset(buffer, 0, sizeof(buffer));

    // Send a response back to the client
    // send(client_info->socket, "Message received", strlen("Message
    // received"),
    //      0);
  }

  // Close the socket and free the thread's resources
  close(client_info->socket);
  free(client_info);
  pthread_exit(NULL);
}

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Create socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Set socket options to allow reuse of address and port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Bind the socket to localhost and port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // Listen for incoming connections
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d...\n", PORT);

  int max_clients = MAX_CLIENTS;
  pthread_t tid[MAX_CLIENTS];
  ClientList client_list;
  client_list.num_clients = 0;

  while (1) {
    // Accept a new connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    // Create a new ClientInfo structure to store client information
    ClientInfo *info = malloc(sizeof(ClientInfo));
    info->socket = new_socket;
    info->address = address;

    pthread_t thread_id;
    client_list.clients[client_list.num_clients++] = info;

    // Create a new thread to handle the client
    pthread_create(&thread_id, NULL, handle_client, (void *)&client_list);
  }

  return 0;
}
