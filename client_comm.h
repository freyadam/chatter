
#ifndef CLIENT_COMM_H_
#define CLIENT_COMM_H_

int get_connected_socket(char * server_address, int server_port);
int run_client(char *  server_address, int server_port);
int process_server_request(int fd);
int process_client_request(int server_fd, int line_fd);

char * username;

#endif // CLIENT_COMM_H_
