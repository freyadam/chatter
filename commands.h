
#ifndef COMMANDS_HEADER_H_
#define	COMMANDS_HEADER_H_

int load_commands(char * filename);
void list_commands();
int perform_command(int fd, char * cmd, char * room_name);
char * get_command(char * cmd_name);
void free_cmd_structs();

struct command_str {
		char * name;
		char * command;
		struct command_str * next;
};

typedef struct command_str cmd_str;

extern cmd_str * commands;
extern pthread_mutex_t commands_mx;
char * cmd_file;

#endif // COMMANDS_HEADER_H_
