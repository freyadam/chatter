
#ifndef MENU_H_
#define MENU_H_

void * run_menu_thread( void * arg);
void create_menu_thread();
void process_comm_request_(struct pollfd ** fds, int * fds_size, char * room_name);
void process_client_request_(struct pollfd ** fds, int * fds_size, int client_no);
int add_client_to_menu(struct pollfd ** fds_ptr, int * fds_size, int fd);

#endif // MENU_H_
