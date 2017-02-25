
#ifndef USERS_HEADER_H_
#define USERS_HEADER_H_

int load_users_from_file(char * filename);
void list_users();
int insert_user(char * users_file, char * username, char * passwd);
int user_present(char * username, char * passwd);
int username_present(char * username, char * passwd);

struct user_pass {
  char * username;
  char * passwd;
  struct user_pass * next;
};

pthread_mutex_t users_mx;
struct user_pass * users;

#endif //USERS_HEADER_H_
