
#ifndef ACCEPT_HEADER_H_
#define ACCEPT_HEADER_H_

int get_listening_socket(int server_port);
void run_accept_thread(int server_port);
void * create_accept_thread();

#endif // ACCEPT_HEADER_H_
