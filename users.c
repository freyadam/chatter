
#include "common.h"
#include "users.h"

pthread_mutex_t users_mx;
user_pass_str * users;

static int load_users_from_file_aux(char * filename) {

	FILE * file = fopen(filename, "r");
	if (file == NULL) {
		printf("File %s could not be opened\n", filename);
		exit(1);
	}

	char * save_ptr, * username, * passwd, * line = NULL;
	size_t len = 0;

	user_pass_str * new;
	while (users != NULL) { // delete current user list

		new = users->next;
		free(users->username);
		free(users->passwd);
		free(users);

		users = new;
	}

	user_pass_str * usr = users;

	errno = 0;
	while (getline(&line, &len, file) != -1) {

		if (line[0] == '#') // comment
			continue;

		username = NULL;
		passwd = NULL;
		save_ptr = NULL;

		username = strtok_r(line, " ", &save_ptr);
		if (username == NULL || username[0] == '\0') {
			printf("Malformed line in user file, skipping...\n");
			continue;
		}

		passwd = strtok_r(NULL, " ", &save_ptr);
		if (passwd == NULL || passwd[0] == '\0') {
			printf("Malformed line in user file, skipping...\n");
			continue;
		}

		if (passwd[ strlen(passwd) - 1] == '\n') {
			passwd[ strlen(passwd) - 1] = '\0';
		}

		// copy username and password into their own storage places
		char * username_storage, * passwd_storage;

		username_storage = strdup(username);
		passwd_storage = strdup(passwd);

		user_pass_str * new
		= (user_pass_str *) malloc(sizeof (user_pass_str));
		new->username = username_storage;
		new->passwd = passwd_storage;
		new->next = NULL;

		if (users == NULL) {
			users = new;
			usr = new;
		} else {
			usr->next = new;
			usr = new;
		}

	}

	if (errno != 0)
		err(1, "getline");

	fclose(file);

	free(line);

	return (0);
}

int load_users_from_file(char * filename) {
	pthread_mutex_lock(&users_mx);

	int result = load_users_from_file_aux(filename);

	pthread_mutex_unlock(&users_mx);

	return (result);
}


static void list_users_aux() {

	user_pass_str * user = users;
	while (user != NULL) {
		printf("User: %s Pass: '%s'\n", user->username, user->passwd);
		user = user->next;
	}

}

void list_users() {
	pthread_mutex_lock(&users_mx);

	list_users_aux();

	pthread_mutex_unlock(&users_mx);
}

static int insert_user_aux(char * users_file, char * username, char * passwd) {

	// check if the username is already taken
	user_pass_str * user = users;
	while (user != NULL) {

		if (strcmp(user->username, username) == 0)
			return (false);

		user = user->next;
	}

	// add new user to file
	int user_fd;
	if ((user_fd = open(users_file, O_WRONLY | O_APPEND, 0666)) == -1)
		err(1, "open -- user file");

	write(user_fd, username, strlen(username));
	write(user_fd, " ", 1);
	write(user_fd, passwd, strlen(passwd));
	write(user_fd, "\n", 1);

	close(user_fd);

	// reload list
	load_users_from_file(users_file);

	return (true);
}

int insert_user(char * users_file, char * username, char * passwd) {
		pthread_mutex_lock(&users_mx);

		int result = insert_user_aux(users_file, username, passwd);

		pthread_mutex_unlock(&users_mx);

		return (result);
}

static int user_present_aux(char * username, char * passwd) {

	user_pass_str * user = users;
	while (user != NULL) {

		if (strcmp(user->username, username) == 0 &&
				strcmp(user->passwd, passwd) == 0)
			return (true);

		user = user->next;

	}

	return (false);
}

int user_present(char * username, char * passwd) {

	pthread_mutex_lock(&users_mx);

	int result = user_present_aux(username, passwd);

	pthread_mutex_unlock(&users_mx);

	return (result);
}

static int username_present_aux(char * username, char * passwd) {

	user_pass_str * user = users;
	while (user != NULL) {

		if (strcmp(user->username, username) == 0)
			return (true);

		user = user->next;

	}

	return (false);
}

int username_present(char * username, char * passwd) {

	pthread_mutex_lock(&users_mx);

	int result = username_present_aux(username, passwd);

	pthread_mutex_unlock(&users_mx);

	return (result);
}

void free_user_structs() {

	pthread_mutex_lock(&users_mx);
	
	user_pass_str * next_usr, * usr = users;
	while (usr != NULL) {
		free(usr->username);
		free(usr->passwd);
		next_usr = usr->next;
		free(usr);
		usr = next_usr;
	}

	pthread_mutex_unlock(&users_mx);

}
