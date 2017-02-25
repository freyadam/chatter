
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

struct command_str {
  char * name;
  char * command;
  struct command_str * next;
};

struct room_str {
  char * room_name;
  struct room_str * next;
};

struct user_pass * users;
struct command_str * commands;
struct room_str * rooms;

#endif //USERS_HEADER_H_
