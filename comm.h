
#ifndef COMM_H_
#define COMM_H_


int add_client(struct pollfd ** fds, int fds_size, int fd);
int delete_client(struct pollfd * fds, int fds_size, int client_no);
void * run_comm_thread(void * arg_struct);
void create_comm_thread(char * name);

#endif // COMM_H_
