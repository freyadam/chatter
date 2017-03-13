
#include "common.h"
#include "menu.h"
#include "proto.h"
#include "thread_common.h"
#include "rooms.h"
#include "users.h"

static void process_comm_request(struct comm_block * room_info,
		char * room_name);
static void process_client_request(struct comm_block * room_info,
		int client_no);

void * run_menu_thread(void * arg_struct) {

	struct new_thread_args * args = (struct new_thread_args *) arg_struct;

	int comm_fd = args->comm_fd;
	int priority_fd = args->priority_fd;
	char * room_name = args->data_ptr->name;

	free(args);

	int fds_size = 2; // priority channel + thread communication channel
	struct pollfd * fds = malloc(sizeof (struct pollfd) * fds_size);
	char ** names = malloc(sizeof (char *) * fds_size);

	struct comm_block room_info;

	// initialize pollfd for priority channel
	init_pollfd_record(&fds[0], priority_fd);
	names[0] = "priority";

	// initialize pollfd for thread communication channel
	init_pollfd_record(&fds[1], comm_fd);
	names[1] = "comm";

	// initialize room_info
	room_info.fds = &fds;
	room_info.names = &names;
	room_info.size = &fds_size;

	int err_poll, client_no;

	while (true) {

		if ((err_poll = poll(fds, fds_size, -1)) < 0)
			errx(1, "poll");

		// priority channel
		if (fds[0].revents & POLLIN) {

			process_priority_request(&room_info, room_name);

		}

		// communication between threads
		if (fds[1].revents & POLLIN) {

			process_comm_request(&room_info, room_name);

		}

		// client threads
		for (client_no = 2; client_no < fds_size; client_no++) {

			if (fds[client_no].revents & POLLIN) {

				process_client_request(&room_info, client_no);

			}

		}

	}

	pthread_exit(NULL);

}

void create_menu_thread() {

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
	thr_data->name = "Menu";
	thr_data->next = NULL;

	// ----- APPEND TO LIST -----

	// get pointer to the last element of thread_list
	if (pthread_mutex_lock(&thr_list_mx) != 0)
		errx(1, "pthread_mutex_lock");

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

	if (pthread_create(&(thr_ptr->id),
		NULL, &run_menu_thread, (void *) args) != 0)
		errx(1, "pthread_create");

}


void print_info_to_new_client(int fd) {

	// send chatroom info to new user
	send_message(fd, "----- Connected to Menu -----");

	// list rooms
	send_message(fd, "Rooms:");

	// skip menu and then list all standard rooms
	int counter = 1;
	int room_str_len;

	struct thread_data * thr_ptr;
	for (thr_ptr = thread_list->next;
		thr_ptr != NULL; thr_ptr = thr_ptr->next) {
		room_str_len = 10 + 3 + strlen(thr_ptr->name) + 1;

		// make sure that room name is not too long
		assert(100 > room_str_len);

		send_message_f(fd, "%d - %s", counter++, thr_ptr->name);

	}

	// list options
	send_message(fd, "Options:");

	send_message(fd, "u - Create new user");

	send_message(fd, "c - Create new chatroom");

	// how to pick your setting
	send_message(fd, "To select your option "
		"simply type its associated number or letter.");

}

int add_client_to_menu(struct comm_block * room_info, char * username, int fd) {

	struct pollfd ** fds_ptr = room_info->fds;
	char *** names = room_info->names;
	int * fds_size = room_info->size;

	*fds_ptr = (struct pollfd *) realloc(*fds_ptr,
		sizeof (struct pollfd) * ((*fds_size)+1));

	if (*fds_ptr == NULL)
		err(1, "realloc");

	*names = (char **) realloc(*names, sizeof (char *) * ((*fds_size)+1));
	if (*names == NULL)
		err(1, "realloc");

	init_pollfd_record(&((*fds_ptr)[*fds_size]), fd);

	(*names)[*fds_size] = username;

	print_info_to_new_client(fd);

	(*fds_size)++;

	return (0);

}

static void process_comm_request(struct comm_block * room_info,
		char * room_name) {

	struct pollfd ** fds = room_info->fds;

	int new_fd;
	char * message, * new_username;
	message = NULL;

	// add new client
	enum dispatch_t type = get_dispatch((*fds)[1].fd, &message);
	if (type != MSG) {
		errx(1, "get_dispatch");
	}

	new_username = strdup(message);

	free(message);
	message = NULL;

	type = get_dispatch((*fds)[1].fd, &message);
	if (type == MSG) {
		new_fd = strtol(message, NULL, 10);
		if (new_fd == 0 && errno == EINVAL)
						err(1, "strtol");
	} else {
		errx(1, "get_dispatch");
	}

	printf("New client: %s -- %d\n", new_username, new_fd);

	// add the new client
	add_client_to_menu(room_info, new_username, new_fd);

	// release allocated resources
	if (message != NULL)
					free(message);

}


static void process_client_request(struct comm_block * room_info,
		int client_no) {


	struct pollfd ** fds = room_info->fds;

	int client_fd;
	char * message;
	message = NULL;
	client_fd = (*fds)[client_no].fd;

	enum dispatch_t type = get_dispatch(client_fd, &message);

	switch (type) {
	case FAILURE:
		errx(1, "get_dispatch");
		break;
	case EOF_STREAM:
		// end (*fds)[client_no];
		(*fds)[client_no].events = 0;
		break;
	default:
		printf("Dispatch received\n");
	}

	switch (type) {
	case ERR:
		send_message(client_fd,
		"Error message received.");
		break;
	case EXT:
		// back to menu
		send_message(client_fd,
		"EXT: Nowhere to exit to from menu.");
		break;
	case END:
		// end connection
		printf(
		"Closing connection for client %d\n", client_no);
		delete_client(room_info, client_no);
		break;
	case CMD:
		// perform cmd
		send_message(client_fd,
		"CMD: This function is disabled in menu.");
		break;
	case MSG:

		if (strcmp(message, "u") == 0) {

			char * username, * passwd;
			username = NULL; passwd = NULL;
			send_message(client_fd,
		"User's name:");
			if (get_message(client_fd, &username) == 0) {
				send_message(client_fd,
		"User's password:");
				if (get_message(client_fd, &passwd)
					== 0) {
					if (insert_user(user_file,
		username, passwd) == true)
						send_message(client_fd,
		"User added.");
					else
						send_message(client_fd,
		"Failed to add new user");
				} else
					send_message(client_fd,
		"Failed to add new user");
			} else
				send_message(client_fd,
		"Failed to add new user");

		} else if (strcmp(message, "c") == 0) {

			send_message(client_fd, "New room's name:");
			char * new_room_name = NULL;
			if (get_message(client_fd, &new_room_name)
				!= 0)
				send_message(client_fd,
		"Failed to add new room");
			else {
				insert_room(room_file, new_room_name);
				send_message(client_fd,
		"Added new room");
			}
			print_info_to_new_client(client_fd);

		} else { // enter chat room

			errno = 0;
			int chat_no = strtol(message, NULL, 10);
			if (errno != 0 || chat_no <= 0) {
				send_message(client_fd,
		"Invalid message - no option selected.");
				return;
			}
			// select which chat room to enter
			struct thread_data * thr_ptr;
			for (thr_ptr = thread_list->next;
		thr_ptr != NULL && chat_no-- > 1; thr_ptr = thr_ptr->next) {}
			if (chat_no >= 1) {
				send_message(client_fd,
		"Such a room does not exist, the number is too high.");
				return;
			}
			// transfer client to the chat room
			printf("%s\n", thr_ptr->name);
			if (transfer_client(thr_ptr->comm_fd,
		room_info, client_no) != 0)
				errx(1, "transfer_client");

		}
		break;
	default:
		assert(false);
	}

				// release allocated resources
				if (message != NULL)
					free(message);


}
