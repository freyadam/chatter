
#ifndef ACCEPT_HEADER_H_
#define	ACCEPT_HEADER_H_

int get_listening_socket(unsigned short server_port);
void * run_accept_thread(void * arg);
pthread_t create_accept_thread(int server_port);

#define MAX_AUTH_THREADS 2

#endif // ACCEPT_HEADER_H_
