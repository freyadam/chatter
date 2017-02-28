
#include "common.h"
#include "proto.h"
#include "thread_common.h"

void init_pollfd_record(struct pollfd ** fds_ptr, int no_of_record, int fd) {

	(*fds_ptr)[no_of_record].fd = fd;
	(*fds_ptr)[no_of_record].events = POLLIN;
	(*fds_ptr)[no_of_record].revents = 0;

}

int transfer_client(int room_fd, struct pollfd ** fds,
		char *** names, int * fds_size, int client_no) {

	int i, client_fd = (*fds)[client_no].fd;
	char * client_no_char = malloc(25);

	send_message(room_fd, (*names)[client_no]);
	snprintf(client_no_char, 10, "%d", client_fd);
	send_message(room_fd, client_no_char);

	for (i = client_no+1; i < *fds_size; i++) {
		(*fds)[i-1] = (*fds)[i];
		(*names)[i-1] = (*names)[i];
	}

	(*fds_size)--;

	*fds = realloc(*fds, sizeof (struct pollfd) * (*fds_size));
	if (*fds == NULL)
		err(1, "realloc");

	*names = realloc(*names, sizeof (char *) * (*fds_size));
	if (*names == NULL)
		err(1, "realloc");

	free(client_no_char);

	return (EXIT_SUCCESS);
}

int delete_client(struct pollfd ** fds, char *** names,
		int * fds_size, int client_no) {

	int i;
	char * client_name = malloc(50 + strlen((*names)[client_no]));

	close((*fds)[client_no].fd);

	for (i = 2; i < *fds_size; i++) {
		if (i != client_no) {
		snprintf(client_name, strlen("User %s left the room.")
		+ strlen((*names)[client_no]) + 1,
		"User %s left the room.", (*names)[client_no]);
			send_message((*fds)[i].fd, client_name);
		}
	}

	for (i = client_no+1; i < *fds_size; i++) {
		(*fds)[i-1] = (*fds)[i];
		(*names)[i-1] = (*names)[i];
	}

	(*fds_size)--;

	*fds = realloc(*fds, sizeof (struct pollfd) * (*fds_size));
	if (*fds == NULL)
		err(1, "realloc");

	free(client_name);

	return (EXIT_SUCCESS);
}

void process_priority_request(struct pollfd * fds,
		int fds_size, char * room_name) {

	char * prefix, * message;
	int client_no;
	prefix = NULL; message = NULL;
	get_dispatch(fds[0].fd, &prefix, &message);


	if (message != NULL)
		printf("Priority message received in %s: %s _ %s\n",
		room_name, prefix, message);
	else
		printf("Priority message received in %s: %s\n",
		room_name, prefix);

	if (strcmp(prefix, "END") == 0) {
		for (client_no = 2; client_no < fds_size;
		client_no++) { // send end to all clients
			send_end(fds[client_no].fd);
		}
		free(prefix); free(message);
		pthread_exit(NULL);
	}
	free(prefix); free(message);

}
