#ifndef COMM_H_
#define	COMM_H_

int add_client(struct comm_block * room_info,
int fd, char * user_name, char * room_name);
void * run_comm_thread(void * arg_struct);
void create_comm_thread(char * name);

#endif // COMM_H_
