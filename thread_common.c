
#include "common.h"
#include "proto.h"
#include "thread_common.h"

void init_pollfd_record(struct pollfd * fd_ptr, int fd) {

	fd_ptr->fd = fd;
	fd_ptr->events = POLLIN;
	fd_ptr->revents = 0;

}

int transfer_client(int room_fd, struct pollfd ** fds,
		char *** names, int * fds_size, int client_no) {

	int i, client_fd = (*fds)[client_no].fd;
	char client_no_char[10];

	send_message(room_fd, (*names)[client_no]);
        send_message_f(room_fd, "%d", client_fd);
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

	return (0);
}

int delete_client(struct pollfd ** fds, char *** names,
		int * fds_size, int client_no) {

        int i, client_name_str_len = 50 + strlen((*names)[client_no]);
	char * client_name = malloc(client_name_str_len);

	close((*fds)[client_no].fd);

	for (i = 2; i < *fds_size; i++) {
		if (i != client_no) {
		snprintf(client_name, client_name_str_len,
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

	return (0);
}

void process_priority_request(struct pollfd * fds,
		int fds_size, char * room_name) {

        char * message = NULL;
	int client_no;
	enum dispatch_t type = get_dispatch(fds[0].fd, &message);

        if (type == FAILURE || type == EOF_STREAM)
          err(1,"process_priority_request");

	if (type == MSG)
		printf("Priority message received in %s: %s\n",
		room_name, message);
	else
		printf("Priority message received in %s\n",
		room_name);

	if (type == END) {
		for (client_no = 2; client_no < fds_size;
		client_no++) { // send end to all clients
			send_end(fds[client_no].fd);
		}
		free(message);
		pthread_exit(NULL);
	}

	free(message);

}
