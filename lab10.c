  1 #include <arpa/inet.h>
  2 #include <stdio.h>
  3 #include <stdlib.h>
  4 #include <string.h>
  5 #include <sys/socket.h>
  6 #include <unistd.h>
  7
  8 #define PORT 8001
  9 #define BUF_SIZE 1024
 10 #define ADDR "127.0.0.1"
 11
 12 #define handle_error(msg)                                                      \
 13 ┆ do {                                                                         \
 14 ┆ ┆ perror(msg);                                                               \
 15 ┆ ┆ exit(EXIT_FAILURE);                                                        \
 16 ┆ } while (0)
 17
 18 #define NUM_MSG 5
 19
 20 static const char *messages[NUM_MSG] = {"Hello", "Apple", "Car", "Green",
 21 ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ "Dog"};
 22
 23 int main() {
 24 ┆ int sfd = socket(AF_INET, SOCK_STREAM, 0);
 25 ┆ if (sfd == -1) {
 26 ┆ ┆ handle_error("socket");
 27 ┆ }
 28 ┆
 29 ┆ struct sockaddr_in addr;
 30 ┆ memset(&addr, 0, sizeof(struct sockaddr_in));
 31 ┆ addr.sin_family = AF_INET;
 32 ┆ addr.sin_port = htons(PORT);
 33 ┆ if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
 34 ┆ ┆ handle_error("inet_pton");
 35 ┆ }
 36 ┆
 37 ┆ if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
 38 ┆ ┆ handle_error("connect");
 39 ┆ }
 40 ┆
 41 ┆ char buf[BUF_SIZE];
 42 ┆ for (int i = 0; i < NUM_MSG; i++) {
 43 ┆ ┆ sleep(1);
 44 ┆ ┆ // prepare message
 45 ┆ ┆ // this pads the desination with NULL
 46 ┆ ┆ strncpy(buf, messages[i], BUF_SIZE);
 47 ┆ ┆
 48 ┆ ┆ if (write(sfd, buf, BUF_SIZE) == -1) {
 49 ┆ ┆ ┆ handle_error("write");
 50 ┆ ┆ } else {
 51 ┆ ┆ ┆ printf("Sent: %s\n", messages[i]);
 52 ┆ ┆ }
 53 ┆ }
 54 ┆
 55 ┆ exit(EXIT_SUCCESS);
 56 }
~

   1 /*
    2 Questions to answer at top of server.c:
    3 (You should not need to change client.c)
    4
    5 Understanding the Client:
    6 1. How is the client sending data to the server? What protocol?
    7 The client is using TCP (seen by the use of SOCK_STREAM).
    8
    9 2. What data is the client sending to the server?
   10 Hello, Apple, Car, Green, Dog sequentially
   11
   12 Understanding the Server:
   13 1. Explain the argument that the `run_acceptor` thread is passed as an argument.
   14 It receives struct acceptor_args * which has atomic_bool run to say whether the
   15 acceptor should keep running, *list_handle that points to the linked list, and a
   16 *list_lock mutex for synchronization for the linked list.
   17
   18 2. How are received messages stored?
   19 A new list_node is created for each message and that node is added to a shared
   20 linked list.
   21
   22 3. What does `main()` do with the received messages?
   23 It waits for enough messages to arrive, and then interates through the list and
   24 prints the messages.
   25
   26 4. How are threads used in this sample code?
   27 There's an acceptor thread to accept new connections, and client threads for
   28 each connected client to read messages.
   29
   30 */
   31
   32 #include <arpa/inet.h>
     33 #include <errno.h>
   34 #include <fcntl.h>
   35 #include <pthread.h>
   36 #include <stdatomic.h>
   37 #include <stdbool.h>
   38 #include <stdio.h>
   39 #include <stdlib.h>
   40 #include <string.h>
   41 #include <sys/socket.h>
   42 #include <unistd.h>
   43 #define BUF_SIZE 1024
   44 #define PORT 8001
   45 #define LISTEN_BACKLOG 32
   46 #define MAX_CLIENTS 4
   47 #define NUM_MSG_PER_CLIENT 5
   48
   49 #define handle_error(msg)                                                      \
   50 ┆ do {                                                                         \
   51 ┆ ┆ perror(msg);                                                               \
   52 ┆ ┆ exit(EXIT_FAILURE);                                                        \
   53 ┆ } while (0)
   54
   55 struct list_node {
   56 ┆ struct list_node *next;
   57 ┆ void *data;
   58 };
   59
   60 struct list_handle {
   61 ┆ struct list_node *last;
   62 ┆ volatile uint32_t count;
  63 };
   64
   65 struct client_args {
   66 ┆ atomic_bool run;
   67 ┆
   68 ┆ int cfd;
   69 ┆ struct list_handle *list_handle;
   70 ┆ pthread_mutex_t *list_lock;
   71 };
   72
   73 struct acceptor_args {
   74 ┆ atomic_bool run;
   75 ┆
   76 ┆ struct list_handle *list_handle;
   77 ┆ pthread_mutex_t *list_lock;
   78 };
   79
   80 int init_server_socket() {
   81 ┆ struct sockaddr_in addr;
   82 ┆
   83 ┆ int sfd = socket(AF_INET, SOCK_STREAM, 0);
   84 ┆ if (sfd == -1) {
   85 ┆ ┆ handle_error("socket");
   86 ┆ }
   87 ┆
   88 ┆ memset(&addr, 0, sizeof(struct sockaddr_in));
   89 ┆ addr.sin_family = AF_INET;
   90 ┆ addr.sin_port = htons(PORT);
   91 ┆ addr.sin_addr.s_addr = htonl(INADDR_ANY);
   92 ┆
  93 ┆ if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
   94 ┆ ┆ handle_error("bind");
   95 ┆ }
   96 ┆
   97 ┆ if (listen(sfd, LISTEN_BACKLOG) == -1) {
   98 ┆ ┆ handle_error("listen");
   99 ┆ }
  100 ┆
  101 ┆ return sfd;
  102 }
  103
  104 // Set a file descriptor to non-blocking mode
  105 void set_non_blocking(int fd) {
  106 ┆ int flags = fcntl(fd, F_GETFL, 0);
  107 ┆ if (flags == -1) {
  108 ┆ ┆ perror("fcntl F_GETFL");
  109 ┆ ┆ exit(EXIT_FAILURE);
  110 ┆ }
  111 ┆ if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
  112 ┆ ┆ perror("fcntl F_SETFL");
  113 ┆ ┆ exit(EXIT_FAILURE);
  114 ┆ }
  115 }
  116
  117 void add_to_list(struct list_handle *list_handle, struct list_node *new_node) {
  118 ┆ struct list_node *last_node = list_handle->last;
  119 ┆ last_node->next = new_node;
  120 ┆ list_handle->last = last_node->next;
  121 ┆ list_handle->count++;
  122 }
  123
  124 int collect_all(struct list_node head) {
  125 ┆ struct list_node *node = head.next; // get first node after head
  126 ┆ uint32_t total = 0;
  127 ┆
  128 ┆ while (node != NULL) {
  129 ┆ ┆ printf("Collected: %s\n", (char *)node->data);
  130 ┆ ┆ total++;
  131 ┆ ┆
  132 ┆ ┆ // Free node and advance to next item
  133 ┆ ┆ struct list_node *next = node->next;
  134 ┆ ┆ free(node->data);
  135 ┆ ┆ free(node);
  136 ┆ ┆ node = next;
  137 ┆ }
  138 ┆
  139 ┆ return total;
  140 }
  141
  142 static void *run_client(void *args) {
  143 ┆ struct client_args *cargs = (struct client_args *)args;
  144 ┆ int cfd = cargs->cfd;
  145 ┆ set_non_blocking(cfd);
  146 ┆
  147 ┆ char msg_buf[BUF_SIZE];
  148 ┆
  149 ┆ while (cargs->run) {
  150 ┆ ┆ ssize_t bytes_read = read(cfd, &msg_buf, BUF_SIZE);
  151 ┆ ┆ if (bytes_read == -1) {
  152 ┆ ┆ ┆ if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
   153 ┆ ┆ ┆ ┆ perror("Problem reading from socket!\n");
  154 ┆ ┆ ┆ ┆ break;
  155 ┆ ┆ ┆ }
  156 ┆ ┆ } else if (bytes_read > 0) {
  157 ┆ ┆ ┆ // Create node with data
  158 ┆ ┆ ┆ struct list_node *new_node = malloc(sizeof(struct list_node));
  159 ┆ ┆ ┆ new_node->next = NULL;
  160 ┆ ┆ ┆ new_node->data = malloc(BUF_SIZE);
  161 ┆ ┆ ┆ memcpy(new_node->data, msg_buf, BUF_SIZE);
  162 ┆ ┆ ┆
  163 ┆ ┆ ┆ struct list_handle *list_handle = cargs->list_handle;
  164 ┆ ┆ ┆ pthread_mutex_lock(cargs->list_lock);
  165 ┆ ┆ ┆ add_to_list(list_handle, new_node);
  166 ┆ ┆ ┆ pthread_mutex_unlock(cargs->list_lock);
  167 ┆ ┆ }
  168 ┆ }
  169 ┆
  170 ┆ if (close(cfd) == -1) {
  171 ┆ ┆ perror("client thread close");
  172 ┆ }
  173 ┆ return NULL;
  174 }
  175
  176 static void *run_acceptor(void *args) {
  177 ┆ int sfd = init_server_socket();
  178 ┆ set_non_blocking(sfd);
  179 ┆
  180 ┆ struct acceptor_args *aargs = (struct acceptor_args *)args;
  181 ┆ pthread_t threads[MAX_CLIENTS];
  182 ┆ struct client_args client_args[MAX_CLIENTS];
  183 ┆
  184 ┆ printf("Accepting clients...\n");
  185 ┆
  186 ┆ uint16_t num_clients = 0;
  187 ┆ while (aargs->run) {
  188 ┆ ┆ if (num_clients < MAX_CLIENTS) {
  189 ┆ ┆ ┆ int cfd = accept(sfd, NULL, NULL);
  190 ┆ ┆ ┆ if (cfd == -1) {
  191 ┆ ┆ ┆ ┆ if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
  192 ┆ ┆ ┆ ┆ ┆ handle_error("accept");
  193 ┆ ┆ ┆ ┆ }
  194 ┆ ┆ ┆ } else {
  195 ┆ ┆ ┆ ┆ printf("Client connected!\n");
  196 ┆ ┆ ┆ ┆
  197 ┆ ┆ ┆ ┆ client_args[num_clients].cfd = cfd;
  198 ┆ ┆ ┆ ┆ client_args[num_clients].run = true;
  199 ┆ ┆ ┆ ┆ client_args[num_clients].list_handle = aargs->list_handle;
  200 ┆ ┆ ┆ ┆ client_args[num_clients].list_lock = aargs->list_lock;
  201 ┆ ┆ ┆ ┆
  202 ┆ ┆ ┆ ┆ pthread_create(&threads[num_clients], NULL, run_client,
  203 ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆ ┆  &client_args[num_clients]);
  204 ┆ ┆ ┆ ┆ num_clients++;
  205 ┆ ┆ ┆ ┆ printf("Client connected!\n");
  206 ┆ ┆ ┆ }
  207 ┆ ┆ }
  208 ┆ }
  209 ┆
  210 ┆ printf("Not accepting any more clients!\n");
  211 ┆
  212 ┆ // Shutdown and cleanup
  213 ┆ for (int i = 0; i < num_clients; i++) {
  214 ┆ ┆
  215 ┆ ┆ client_args[i].run = false;
  216 ┆ ┆ pthread_join(threads[i], NULL);
  217 ┆ ┆ close(client_args[i].cfd);
  218 ┆ }
  219 ┆
  220 ┆ if (close(sfd) == -1) {
  221 ┆ ┆ perror("closing server socket");
  222 ┆ }
  223 ┆ return NULL;
  224 }
  225
  226 int main() {
  227 ┆ pthread_mutex_t list_mutex;
  228 ┆ pthread_mutex_init(&list_mutex, NULL);
  229 ┆
  230 ┆ // List to store received messages
  231 ┆ // - Do not free list head (not dynamically allocated)
  232 ┆ struct list_node head = {NULL, NULL};
W 233 ┆ struct list_node *last = &head;     ■ unused variable 'last' [-Wunused-variable]
  234 ┆ struct list_handle list_handle = {
  235 ┆ ┆ ┆ .last = &head,
  236 ┆ ┆ ┆ .count = 0,
  237 ┆ };
  238 ┆
  239 ┆ pthread_t acceptor_thread;
  240 ┆ struct acceptor_args aargs = {
  241 ┆ ┆ ┆ .run = true,
  242 ┆ ┆ ┆ .list_handle = &list_handle,
  243 ┆ ┆ ┆ .list_lock = &list_mutex,
  244 ┆ };
  245 ┆ pthread_create(&acceptor_thread, NULL, run_acceptor, &aargs);
  246 ┆
  247 ┆ while (true) {
  248 ┆ ┆
  249 ┆ ┆ pthread_mutex_lock(&list_mutex);
  250 ┆ ┆ uint32_t count = list_handle.count;
  251 ┆ ┆ pthread_mutex_unlock(&list_mutex);
  252 ┆ ┆
  253 ┆ ┆ if (count >= MAX_CLIENTS * NUM_MSG_PER_CLIENT)
  254 ┆ ┆ ┆ break;
  255 ┆ }
  256 ┆
  257 ┆ aargs.run = false;
  258 ┆ pthread_join(acceptor_thread, NULL);
  259 ┆
  260 ┆ if (list_handle.count != MAX_CLIENTS * NUM_MSG_PER_CLIENT) {
  261 ┆ ┆ printf("Not enough messages were received!\n");
  262 ┆ ┆ return 1;
  263 ┆ }
  264 ┆
  265 ┆ int collected = collect_all(head);
  266 ┆ printf("Collected: %d\n", collected);
  267 ┆ if (collected != list_handle.count) {
  268 ┆ ┆ printf("Not all messages were collected!\n");
  269 ┆ ┆ return 1;
  270 ┆ } else {
  271 ┆ ┆ printf("All messages were collected!\n");
  272 ┆ }
  273 ┆
  274 ┆ pthread_mutex_destroy(&list_mutex);
  275 ┆
  276 ┆ return 0;
  277 }
~
