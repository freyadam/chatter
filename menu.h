
#ifndef MENU_H_
#define MENU_H_

void * run_menu_thread( void * arg);
void create_menu_thread();
int add_client_to_menu(struct pollfd ** fds_ptr, int * fds_size, int fd);

#endif // MENU_H_
