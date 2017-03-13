
#ifndef THREAD_COMMON_H_
#define	THREAD_COMMON_H_

void init_pollfd_record(struct pollfd * fd_ptr, int fd);
int delete_client(struct comm_block * room_info, int client_no);
void process_priority_request(struct comm_block * room_info, char * room_name);
int transfer_client(int room_fd, struct comm_block * room_info, int client_no);

extern __thread int end_of_thread;

#endif // THREAD_COMMON_H_
