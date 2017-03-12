
#include "common.h"
#include "commands.h"
#include "proto.h"

cmd_str * commands;
pthread_mutex_t commands_mx;

static int load_commands_aux(char * filename) {


	FILE * file = fopen(filename, "r");
	if (file == NULL) {
		printf("File %s could not be opened\n", filename);
		exit(1);
	}

	char * save_ptr, * cmd_name, * cmd, * line = NULL;
	size_t len = 0;

	cmd_str * new;
	while (commands != NULL) { // delete current command list

		new = commands->next;

		free(commands);

		commands = new;
	}

	cmd_str * end_of_cmd_list = commands;

	errno = 0;
	while (getline(&line, &len, file) != -1) {

		if (line[0] == '#') // comment
			continue;

		cmd_name = NULL;
		cmd = NULL;
		save_ptr = NULL;

		cmd_name = strtok_r(line, " ", &save_ptr);
		if (cmd_name == NULL || cmd_name[0] == '\0') {
			printf("Malformed line in commands file, "
		"skipping...\n");
			continue;
		}

		cmd = line + strlen(cmd_name) + 1;
		if (cmd == NULL || cmd[0] == '\0') {
			printf("Malformed line in commands file, "
		"skipping...\n");
			continue;
		}

		if (cmd[ strlen(cmd) - 1] == '\n') {
			cmd[ strlen(cmd) - 1] = '\0';
		}

		// copy name and actual command into their own storage places
		char * name_storage, * cmd_storage;

		cmd_storage = strdup(cmd);
		name_storage = strdup(cmd_name);

		cmd_str * new = (cmd_str *)
		malloc(sizeof (cmd_str));
		new->name = name_storage;
		new->command = cmd_storage;
		new->next = NULL;

		if (commands == NULL) {
			commands = new;
			end_of_cmd_list = new;
		} else {
			end_of_cmd_list->next = new;
			end_of_cmd_list = new;
		}

	}

	if (errno != 0)
		err(1, "getline");

	free(line);

	return (0);
}

int load_commands(char * filename) {

	pthread_mutex_lock(&commands_mx);

	int result = load_commands_aux(filename);

	pthread_mutex_unlock(&commands_mx);

	return (result);

}

static void list_commands_aux() {

	cmd_str * cmd = commands;
	while (cmd != NULL) {

		printf("%s -> '%s'\n", cmd->name, cmd->command);

		cmd = cmd->next;
	}

}

void list_commands() {

	pthread_mutex_lock(&commands_mx);

	list_commands_aux();

	pthread_mutex_unlock(&commands_mx);

}

static int perform_command_aux(int fd, char * cmd, char * room_name) {

        int tmp_str_len = strlen("/tmp/") + 6 + strlen(room_name) + 1;
	char * temp = malloc(tmp_str_len);
        if (temp == NULL) {
          err(1, "malloc - perform_command_aux");
        }

        snprintf(temp, tmp_str_len, "/tmp/%d_%s", getpid(), room_name);

	// combination of pid and room name is unique
        int arg_str_len = strlen(cmd) + 3 + strlen(temp) + 1;
		char * arg = malloc(arg_str_len);
		if (arg == NULL) {
				err(1, "malloc - perform_command_aux");
		}

	snprintf(arg, arg_str_len,
		"%s > %s", cmd, temp);

	if (system(arg) == -1)
		send_message(fd, "Error: command failed");
	else {
		send_message_from_file(fd, temp);
	}

	// remove temp file
	unlink(temp);

	free(arg);
	free(temp);

	return (0);
}

int perform_command(int fd, char * cmd, char * room_name) {

	pthread_mutex_lock(&commands_mx);

	int result = perform_command_aux(fd, cmd, room_name);

	pthread_mutex_unlock(&commands_mx);

	return (result);

}

static char * get_command_aux(char * cmd_name) {

	cmd_str * cmd = commands;
	while (cmd != NULL) {

		if (strcmp(cmd_name, cmd->name) == 0)
			return (cmd->command);

		cmd = cmd->next;
	}

	return (NULL);
}

char * get_command(char * cmd_name) {

	pthread_mutex_lock(&commands_mx);

	char * result = get_command_aux(cmd_name);

	pthread_mutex_unlock(&commands_mx);

	return (result);

}
