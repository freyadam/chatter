
#ifndef MENU_H_
#define MENU_H_

void * run_menu_thread( void * arg);
void create_menu_thread();
static void process_comm_request(struct pollfd ** fds, int * fds_size, char * room_name);
static void process_client_request(struct pollfd ** fds, int * fds_size, int client_no);
int add_client_to_menu(struct pollfd ** fds_ptr, int * fds_size, int fd);
int transfer_client(int room_fd, struct pollfd ** fds, int * fds_size, int client_no);

#endif // MENU_H_
