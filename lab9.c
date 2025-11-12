/*
  2 Questions to answer at top of client.c:
  3 (You should not need to change the code in client.c)
  4 1. What is the address of the server it is trying to connect to (IP address and
  5 port number). IP Address is 127.0.0.1 and the port number is 8000
  6 2. Is it UDP or TCP? How do you know?
  7 It is TCP, because it uses SOCK_STREAM and also uses read/write
  8 3. The client is going to send some data to the server. Where does it get this
  9 data from? How can you tell in the code? It gets it from standard input as seen
 10 by the STDIN_FILENO and the use of a buffer to store the data.
 11 4. How does the client program end? How can you tell that in the code?
 12 It ends when the user does CTRL+C and ends the standard input. This is because
 13 of the while loop condition, which will run until read returns 0, which means
 14 stdin is closed.
 15 */
 16
 17 #include <arpa/inet.h>
 18 #include <stdio.h>
 19 #include <stdlib.h>
 20 #include <string.h>
 21 #include <sys/socket.h>
 22 #include <unistd.h>
 23
 24 #define PORT 8000
 25 #define BUF_SIZE 64
 26 #define ADDR "127.0.0.1"
 27
 28 #define handle_error(msg)                                                      \
 29 ┆ do {                                                                         \
 30 ┆ ┆ perror(msg);                                                               \
 31 ┆ ┆ exit(EXIT_FAILURE);                                                        \
 32 ┆ } while (0)
 33
 34 int main() {
 35 ┆ struct sockaddr_in addr;
 36 ┆ int sfd;
 37 ┆ ssize_t num_read;
 38 ┆ char buf[BUF_SIZE];
 39 ┆
 40 ┆ sfd = socket(AF_INET, SOCK_STREAM, 0);
 41 ┆ if (sfd == -1) {
 42 ┆ ┆ handle_error("socket");
 43 ┆ }
 44 ┆
 45 ┆ memset(&addr, 0, sizeof(struct sockaddr_in));
 46 ┆ addr.sin_family = AF_INET;
 47 ┆ addr.sin_port = htons(PORT);
 48 ┆ if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
 49 ┆ ┆ handle_error("inet_pton");
 50 ┆ }
 51 ┆
 52 ┆ int res = connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
 53 ┆ if (res == -1) {
 54 ┆ ┆ handle_error("connect");
 55 ┆ }
 56 ┆
 57 ┆ while ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1) {
 58 ┆ ┆ if (write(sfd, buf, num_read) != num_read) {
 59 ┆ ┆ ┆ handle_error("write");
 60 ┆ ┆ }
 61 ┆ ┆ printf("Just sent %zd bytes.\n", num_read);
 62 ┆ }
 63 ┆
 64 ┆ if (num_read == -1) {
 65 ┆ ┆ handle_error("read");
 66 ┆ }
 67 ┆
 68 ┆ close(sfd);
 69 ┆ exit(EXIT_SUCCESS);
 70 }

 1 #include <arpa/inet.h>
  2 #include <errno.h>
  3 #include <pthread.h>
  4 #include <stdio.h>
  5 #include <stdlib.h>
  6 #include <string.h>
  7 #include <sys/socket.h>
  8 #include <unistd.h>
  9
 10 #define BUF_SIZE 64
 11 #define PORT 8000
 12 #define LISTEN_BACKLOG 32
 13
 14 #define handle_error(msg)                                                      \
 15 ┆ do {                                                                         \
 16 ┆ ┆ perror(msg);                                                               \
 17 ┆ ┆ exit(EXIT_FAILURE);                                                        \
 18 ┆ } while (0)
 19
 20 // Shared counters for: total # messages, and counter of clients (used for
 21 // assigning client IDs)
 22 int total_message_count = 0;
 23 int client_id_counter = 1;
 24
 25 // Mutexs to protect above global state.
 26 pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
 27 pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;
 28
 29 struct client_info {
 30 ┆ int cfd;
 31 ┆ int client_id;
 32 };
 33
 34 void *handle_client(void *arg) {
 35 ┆ struct client_info *client = (struct client_info *)arg;
 36 ┆
 37 ┆ char buf[BUF_SIZE];
 38 ┆ ssize_t num_read;
 39 ┆
 40 ┆ pthread_mutex_lock(&client_id_mutex);
 41 ┆ printf("New client created! ID %d on socket FD %d\n", client->client_id,
 42 ┆ ┆ ┆ ┆  client->cfd);
 43 ┆ pthread_mutex_unlock(&client_id_mutex);
 44 ┆
 45 ┆ while ((num_read = read(client->cfd, buf, BUF_SIZE - 1)) > 0) {
 46 ┆ ┆
 47 ┆ ┆ buf[num_read] = '\0';
 48 ┆ ┆
 49 ┆ ┆ pthread_mutex_lock(&client_id_mutex);
 50 ┆ ┆ total_message_count++;
 51 ┆ ┆ printf("Msg #%4d; Client ID %d: %s", total_message_count, client->client_id,
 52 ┆ ┆ ┆ ┆ ┆  buf);
 53 ┆ ┆ pthread_mutex_unlock(&client_id_mutex);
 54 ┆ }
 55 ┆
 56 ┆ pthread_mutex_lock(&client_id_mutex);
 57 ┆ printf("Ending thread for client %d\n", client->client_id);
 58 ┆ pthread_mutex_unlock(&client_id_mutex);
 59 ┆
 60 ┆ close(client->cfd);
 61 ┆ free(client);
 62 ┆
 63 ┆ return NULL;
 64 }
 65
 66 int main() {
 67 ┆ struct sockaddr_in addr;
 68 ┆ int sfd;
 69 ┆
 70 ┆ sfd = socket(AF_INET, SOCK_STREAM, 0);
 71 ┆ if (sfd == -1) {
 72 ┆ ┆ handle_error("socket");
 73 ┆ }
 74 ┆
 75 ┆ memset(&addr, 0, sizeof(struct sockaddr_in));
 76 ┆ addr.sin_family = AF_INET;
 77 ┆ addr.sin_port = htons(PORT);
 78 ┆ addr.sin_addr.s_addr = htonl(INADDR_ANY);
 79 ┆
 80 ┆ if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
 81 ┆ ┆ handle_error("bind");
 82 ┆ }
 83 ┆
 84 ┆ if (listen(sfd, LISTEN_BACKLOG) == -1) {
 85 ┆ ┆ handle_error("listen");
 86 ┆ }
 87 ┆
 88 ┆ for (;;) {
 89 ┆ ┆
 90 ┆ ┆ struct sockaddr_in client_addr;
 91 ┆ ┆ socklen_t client_addr_len = sizeof(client_addr);
 92 ┆ ┆
 93 ┆ ┆ int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
 94 ┆ ┆
 95 ┆ ┆ struct client_info *info = malloc(sizeof(struct client_info));
 96 ┆ ┆ info->cfd = cfd;
 97 ┆ ┆ int new_client_id = client_id_counter;
 98 ┆ ┆ info->client_id = new_client_id;
 99 ┆ ┆
100 ┆ ┆ pthread_t tid;
101 ┆ ┆ pthread_create(&tid, NULL, handle_client, info);
102 ┆ }
103 ┆
104 ┆ if (close(sfd) == -1) {
105 ┆ ┆ handle_error("close");
106 ┆ }
107 ┆
108 ┆ return 0;
109 }
