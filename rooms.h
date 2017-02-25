
#ifndef ROOMS_HEADER_H_
#define ROOMS_HEADER_H_

int load_rooms(char * filename);
void list_rooms();
int room_present(char * name);
int insert_room(char * filename, char * name);

pthread_mutex_t rooms_mx;

#endif // ROOMS_HEADER_H_
