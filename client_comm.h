
#ifndef CLIENT_COMM_H_
#define CLIENT_COMM_H_

int get_connected_socket(char * server_address, int server_port);
int run_client(char *  server_address, int server_port);
int process_client_request(int fd, char ** line_ptr);

#endif // CLIENT_COMM_H_
