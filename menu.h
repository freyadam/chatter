
#ifndef MENU_H_
#define	MENU_H_

void * run_menu_thread(void * arg);
void create_menu_thread();
int add_client_to_menu(struct comm_block * room_info, char * username, int fd);

#endif // MENU_H_
