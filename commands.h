
#ifndef COMMANDS_HEADER_H_
#define COMMANDS_HEADER_H_

int load_commands(char * filename);
void list_commands();
int perform_command(int fd, char * cmd, char * room_name);
char * get_command(char * cmd_name);

struct command_str {
  char * name;
  char * command;
  struct command_str * next;
};

struct command_str * commands;
pthread_mutex_t commands_mx;
char * cmd_file;

#endif // COMMANDS_HEADER_H_

