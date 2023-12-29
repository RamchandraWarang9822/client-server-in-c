// client.c

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int sock;
char username[MAX_BUFFER_SIZE];

void *send_message(void *arg) {
  char sendBuffer[MAX_BUFFER_SIZE];
  while (1) {
    // Get a message from the user
    fgets(sendBuffer, MAX_BUFFER_SIZE, stdin);
    sendBuffer[strcspn(sendBuffer, "\n")] =
        '\0'; // Remove the newline character

    // Send the message to the server
    send(sock, sendBuffer, strlen(sendBuffer), 0);

    // Clear the buffer
    memset(sendBuffer, 0, sizeof(sendBuffer));
  }
}

void *receive_messages(void *arg) {
  char recieveBuffer[MAX_BUFFER_SIZE];
  while (1) {
    // Receive and print messages from the server
    int valread = read(sock, recieveBuffer, MAX_BUFFER_SIZE);

    // Check if the server disconnected
    if (valread <= 0) {
      printf("Server disconnected\n");
      exit(EXIT_FAILURE);
    }

    // Print the server's response
    printf("%s", recieveBuffer);
    printf("\n");
    // Clear the buffer
    memset(recieveBuffer, 0, sizeof(recieveBuffer));
  }
}

int main() {
  struct sockaddr_in server_addr;
  pthread_t send_thread, receive_thread;

  // Get the user's name
  printf("Enter your name: ");
  fgets(username, MAX_BUFFER_SIZE, stdin);
  username[strcspn(username, "\n")] = '\0'; // Remove the newline character

  // Create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation error");
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    perror("Invalid address/ Address not supported");
    exit(EXIT_FAILURE);
  }

  // Connect to the server
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection Failed");
    exit(EXIT_FAILURE);
  }

  // Send the username to the server
  send(sock, username, strlen(username), 0);
  printf("Connected as: %s\n", username);

  // Create threads for sending and receiving messages
  if (pthread_create(&send_thread, NULL, send_message, NULL) != 0) {
    perror("Error creating send thread");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }

  // Wait for the threads to finish
  pthread_join(send_thread, NULL);
  pthread_join(receive_thread, NULL);

  // Close the socket
  close(sock);

  return 0;
}
