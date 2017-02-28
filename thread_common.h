
#ifndef THREAD_COMMON_H_
#define	THREAD_COMMON_H_

void init_pollfd_record(struct pollfd ** fds_ptr,
int no_of_record, int fd);
int delete_client(struct pollfd ** fds, char *** names,
int * fds_size, int client_no);
void process_priority_request(struct pollfd * fds,
int fds_size, char * room_name);
int transfer_client(int room_fd, struct pollfd ** fds,
char *** names, int * fds_size, int client_no);

#endif // THREAD_COMMON_H_
