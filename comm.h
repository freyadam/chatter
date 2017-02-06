
#ifndef COMM_H_
#define COMM_H_

void * run_comm_thread(void * arg_struct);
void create_comm_thread(int client_fd, char * name);

#endif // COMM_H_
