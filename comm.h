#ifndef COMM_H_
#define COMM_H_

int add_client(struct pollfd ** fds, int * fds_size, int fd);
int delete_client(struct pollfd ** fds, int * fds_size, int client_no);
void * run_comm_thread(void * arg_struct);
void create_comm_thread(char * name);
void process_priority_request(struct pollfd * fds, int fds_size);
void process_comm_request(struct pollfd ** fds, int * fds_size);
void process_client_request(struct pollfd ** fds, int * fds_size, int client_no);
void init_pollfd_record(struct pollfd ** fds_ptr, int no_of_record, int fd);

#endif // COMM_H_
