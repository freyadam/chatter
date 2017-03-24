
#include "common.h"
#include "comm.h"
#include "proto.h"
#include "thread_common.h"
#include "commands.h"

static void process_comm_request(struct comm_block * room_info,
		char * room_name);
static void process_client_request(struct comm_block * room_info,
		int client_no);

void poll_cycle(struct comm_block * room_info, char * room_name) {

	int client_no;

	if (poll(room_info->fds, room_info->size, -1) < 0)
		errx(1, "poll");

	// priority channel
	if (room_info->fds[0].revents & POLLIN) {

		process_priority_request(room_info, room_name);

	}

	// communication between threads
	if (room_info->fds[1].revents & POLLIN) {

		process_comm_request(room_info, room_name);

	}

	// client threads
	for (client_no = 2; client_no < room_info->size; client_no++) {

		if (room_info->fds[client_no].revents & POLLIN) {


			process_client_request(room_info, client_no);

		}

	}

}

void * run_comm_thread(void * arg_struct) {

	struct new_thread_args * args = (struct new_thread_args *) arg_struct;

	int comm_fd = args->comm_fd;
	int priority_fd = args->priority_fd;
	char * room_name = args->data_ptr->name;

	free(args);

	end_of_thread = false;

	int fds_size = 2; // priority channel + thread communication channel
	struct pollfd * fds = malloc(sizeof (struct pollfd) * fds_size);
	char ** names = malloc(sizeof (char *) * fds_size);
	struct comm_block room_info;

	// initialize comm_block
	room_info.fds = fds;
	room_info.names = names;
	room_info.size = fds_size;

	// initialize pollfd for priority channel
	init_pollfd_record(&fds[0], priority_fd);
	names[0] = room_name;

	// initialize pollfd for thread communication channel
	init_pollfd_record(&fds[1], comm_fd);
	names[1] = "comm";

	while (!end_of_thread) {

		poll_cycle(&room_info, room_name);

	}

	free(room_info.fds);
	int i;
	for (i = 2; i < room_info.size; i++) {
		free(room_info.names[i]);
	}
	free(room_info.names);

	free(room_name);

	return (NULL);

}

static void process_comm_request(struct comm_block * room_info,
		char * room_name) {

	int new_fd;
	char * message, * new_username;
	message = NULL;

	// add new client
	enum dispatch_t type = get_dispatch(room_info->fds[1].fd, &message);
	if (type != MSG)
		errx(1, "process_comm_request");

	new_username = message;
	message = NULL;

	type = get_dispatch(room_info->fds[1].fd, &message);
	if (type != MSG)
		errx(1, "process_comm_request");
	else {
		new_fd = strtol(message, NULL, 10);
		if (new_fd == 0 && errno == EINVAL)
			err(1, "strtol");
	}

	printf("New client: %s -- its fd: %d\n", new_username, new_fd);

	// add the new client
	add_client(room_info, new_fd, new_username, room_name);

	// release allocated resources
	if (message != NULL)
		free(message);

}


static void process_client_request(struct comm_block * room_info,
		int client_no) {

	int i, client_fd;
	char * message;

	message = NULL;
	client_fd = room_info->fds[client_no].fd;

	enum dispatch_t type = get_dispatch(client_fd, &message);
	if (type == FAILURE) { // something went wrong
		close(client_no);
		errx(1, "get_dispatch");
	} else if (type == EOF_STREAM) { // EOF
		delete_client(room_info, client_no);
		return;
	}

	else { // everything worked well --> valid message

		printf("Dispatch received\n");

		switch (type) {
		case ERR:
			send_message(client_fd, "Error message received.");
			break;
		case EXT:
			transfer_client(thread_list->comm_fd,
							room_info, client_no);
			break;
		case END:
			// end connection
			printf("Closing connection for client %d\n", client_no);
			delete_client(room_info, client_no);
			break;
		case CMD:
			printf("%s\n", message);

			// perform cmd
			char * cmd = get_command(message);
			if (cmd == NULL) {
				send_message(client_fd, "Command not found.");
			} else {
				perform_command(client_fd, cmd,
    room_info->names[0]); // name of room
			}
			break;
		case MSG:
			// send message to all the other clients
			for (i = 2; i < room_info->size; i++) {
				if (i != client_no)
		send_message_f(room_info->fds[i].fd, "<%s> %s",
		room_info->names[client_no], message);
			}
			break;
		default:
			assert(false);
		}

		// release allocated resources
		if (message != NULL)
			free(message);

	}

}

void send_info_to_new_user(struct comm_block * room_info, char * room_name) {

	int i, fd;

	fd = room_info->fds[room_info->size].fd;

	send_message_f(fd, "----- Connected to room %s -----", room_name);
	if (room_info->size > 2) {

		send_message(fd, "Current users:");
		for (i = 2; i < room_info->size; i++) {
			send_message(fd, room_info->names[i]);
		}

	}

	send_message(fd, "Commands:");
	send_message(fd, "/cmd task ... Perform task on server.");
	send_message(fd, "/ext ... Exit to menu.");
	send_message(fd, "/end ... End connection.");

}

int add_client(struct comm_block * room_info,
		int fd, char * user_name, char * room_name) {

	int i;

	room_info->fds = (struct pollfd *) realloc(room_info->fds,
		sizeof (struct pollfd) * (room_info->size+1));
	if (room_info->fds == NULL)
		err(1, "realloc");

	room_info->names = (char **) realloc(room_info->names,
    sizeof (char *) * (room_info->size+1));
	if (room_info->names == NULL)
		err(1, "realloc");

	init_pollfd_record(&(room_info->fds[room_info->size]), fd);
	room_info->names[room_info->size] = user_name;

	// send chatroom info to new user
	send_info_to_new_user(room_info, room_name);

	// send message to others as well
	for (i = 2; i < room_info->size; i++) {
					send_message_f(room_info->fds[i].fd,
		"New user connected: %s", user_name);
	}

	room_info->size++;

	return (EXIT_SUCCESS);
}

void create_comm_thread(char * name) {

	int comm_pipe[2], priority_pipe[2];
	struct thread_data * thr_data = malloc(sizeof (struct thread_data));
	bzero(thr_data, sizeof (struct thread_data));

	// ----- INITIALIZE STRUCT -----

	if (pipe(comm_pipe) == -1)
		err(1, "pipe");
	thr_data->comm_fd = comm_pipe[1];

	if (pipe(priority_pipe) == -1)
		err(1, "pipe");
	thr_data->priority_fd = priority_pipe[1];

	thr_data->name = name;

	thr_data->next = NULL;

	// ----- APPEND TO LIST -----

	// get pointer to the last element of thread_list
	if (pthread_mutex_lock(&thr_list_mx) != 0)
		errx(1, "pthread_mutex_unlock");

	struct thread_data * thr_ptr;
	if (thread_list == NULL) {
		thread_list = thr_data;
		thr_ptr = thread_list;
	} else {
		for (thr_ptr = thread_list; thr_ptr->next != NULL;
		thr_ptr = thr_ptr->next) {}
		thr_ptr->next = thr_data;
	}

	if (pthread_mutex_unlock(&thr_list_mx) != 0)
		errx(1, "pthread_mutex_unlock");

	// ----- START NEW THREAD -----

	struct new_thread_args * args = malloc(sizeof (struct new_thread_args));
	args->comm_fd = comm_pipe[0];
	args->priority_fd = priority_pipe[0];
	args->data_ptr = thr_data;

	if (pthread_create(&(thr_ptr->id), NULL, &run_comm_thread,
		(void *) args) != 0)
		errx(1, "pthread_create");

	// pthread_detach(thr_ptr->id);

}
