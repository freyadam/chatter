
#include "common.h"
#include "proto.h"
#include "thread_common.h"

__thread int end_of_thread;

void init_pollfd_record(struct pollfd * fd_ptr, int fd) {

	fd_ptr->fd = fd;
	fd_ptr->events = POLLIN;
	fd_ptr->revents = 0;

}

int transfer_client(int room_fd, struct comm_block * room_info, int client_no) {

	struct pollfd ** fds = &(room_info->fds);
	char *** names = &(room_info->names);
	int * fds_size = &(room_info->size);

	int i, client_fd = (*fds)[client_no].fd;

	send_message(room_fd, (*names)[client_no]);

	free((*names)[client_no]);

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

int delete_client(struct comm_block * room_info, int client_no) {

	int i;
	struct pollfd ** fds = &(room_info->fds);
	char *** names = &(room_info->names);
	int * fds_size = &(room_info->size);

	close((*fds)[client_no].fd);
	free((*names)[client_no]);

	for (i = 2; i < *fds_size; i++) {
		if (i != client_no) {
			send_message_f((*fds)[i].fd,
		"User %s left the room.", (*names)[client_no]);
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

	*names = realloc(*names, sizeof (struct pollfd) * (*fds_size));
	if (*names == NULL)
		err(1, "realloc");

	return (0);
}

void process_priority_request(struct comm_block * room_info, char * room_name) {

	struct pollfd * fds = room_info->fds;
	int fds_size = room_info->size;

	char * message = NULL;
	int client_no;
	enum dispatch_t type = get_dispatch(fds[0].fd, &message);

	if (type == FAILURE || type == EOF_STREAM)
		err(1, "process_priority_request");
	if (type == MSG) {
		printf("Priority message received in %s: %s\n",
		room_name, message);
		free(message);
	} else 
		printf("Priority dispatch received in %s\n",
		room_name);

	if (type == CMD) {
		free(message);
	} else if (type == END) {
		for (client_no = 2; client_no < fds_size;
		client_no++) { // send end to all clients
			send_end(fds[client_no].fd);
		}
		end_of_thread = true;
	}

}
