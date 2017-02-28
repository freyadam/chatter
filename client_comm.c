
#include "common.h"
#include "proto.h"
#include "client_comm.h"
#include "thread_common.h"

void init_hints(struct addrinfo * hints_ptr) {

	bzero(hints_ptr, sizeof (*hints_ptr));
	hints_ptr->ai_family = AF_UNSPEC;
	hints_ptr->ai_socktype = SOCK_STREAM;

}

int get_connected_socket(char * server_address, int server_port) {

	int fd;
	char * server_port_string = malloc(5);
	struct addrinfo hints, * result, * addr_info;

	init_hints(&hints);

	snprintf(server_port_string, 8, "%d", server_port);

	if (getaddrinfo(server_address, server_port_string,
		&hints, &result) != 0)
		err(1, "getaddrinfo");

	for (addr_info = result; addr_info != NULL;
		addr_info = addr_info->ai_next) {

		if ((fd = socket(addr_info->ai_family,
		addr_info->ai_socktype, addr_info->ai_protocol)) == -1)
			continue;

		if (connect(fd, addr_info->ai_addr, addr_info->ai_addrlen)
		== -1) {
			close(fd);
			continue;
		}

		break;

	}

	freeaddrinfo(result);

	if (addr_info == NULL)
		err(1, "no valid gettaddrinfo result");

	return (((fd)));

}

void * lines_to_pipe(void * arg) {

	int * pipe = (int *) arg;
	char * line;
	ssize_t read, wr_read;
	size_t line_size = 0;

	while (true) {

		line = NULL;
		line_size = 0;

		while ((read = getline(&line, &line_size, stdin)) != 0) {

			if (read == -1)
				err(1, "getline");

			wr_read = write(*pipe, line, read);
			if (wr_read == -1)
				err(1, "write");
			if (wr_read != read) {
				err(1, "write");
			}

		}

		close(*pipe);

		free(line);

	}

	return (((NULL)));
}

void client_sigint_handler(int sig) {

	assert(sig == SIGINT);

}

void set_sigint_handler() {

	struct sigaction act;
	act.sa_handler = &client_sigint_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(SIGINT, &act, NULL) == -1)
		err(1, "sigaction");

}

void poll_cycle(struct pollfd ** fds_ptr) {

	int err_poll, result;
	struct pollfd * fds = *fds_ptr;

		err_poll = poll(fds, 2, -1);
		if (err_poll == -1 && errno == EINTR) {
			send_end(fds[0].fd);
			printf("Exiting...\n");

			exit(0);
		} else if (err_poll == -1)
			err(1, "poll");

		// input from server
		if (fds[0].revents & POLLIN) {

			result = process_server_request(fds[0].fd);

			if (result == EOF_IN_STREAM) {
				printf("End of transmission\n");
				exit(0);
			} else if (result == EXIT_FAILURE)
				errx(1, "process_server_request");

		}

		// input from client
		if (fds[1].revents & POLLIN) {

			result = process_client_request(fds[0].fd, fds[1].fd);

			if (result == EXIT_FAILURE)
				errx(1, "process_client_request");

		}

}

int run_client(char * server_address, int server_port,
		char * username, char * password) {

	pthread_t get_line_thread;
	int line_pipe[2];

	int server_fd = get_connected_socket(server_address, server_port);

	// send auth info
	send_message(server_fd, username);
	send_message(server_fd, password);

	if (pipe(line_pipe) == -1)
		err(1, "pipe");

	if (pthread_create(&get_line_thread, NULL,
		&lines_to_pipe, &(line_pipe[1])) > 0)
		errx(1, "pthread_create");

	set_sigint_handler();

	struct pollfd * fds = malloc(sizeof (struct pollfd) * 2);

	// initialize pollfd for server
	init_pollfd_record(&fds, 0, server_fd);

	// initialize pollfd for user-input
	init_pollfd_record(&fds, 1, line_pipe[0]);

	while (true) {

		poll_cycle(&fds);

	}

	return (((EXIT_SUCCESS)));
}

int process_server_request(int fd) {

	char * prefix, * message;
	prefix = NULL; message = NULL;

	int err_dispatch = get_dispatch(fd, &prefix, &message);

	if (err_dispatch == EXIT_FAILURE)
		return (((EXIT_FAILURE)));
	else if (err_dispatch == EOF_IN_STREAM)
		return (((EOF_IN_STREAM)));

	if (strcmp(prefix, "ERR") == 0) {

		printf("ERR\n");

	} else if (strcmp(prefix, "EXT") == 0) {

		printf("EXT\n");

	} else if (strcmp(prefix, "END") == 0) {

		return (((EOF_IN_STREAM)));

	} else if (strcmp(prefix, "CMD") == 0) {

		// client doesn't take commands from server
		send_end(fd);
		return (((EXIT_FAILURE)));

	} else if (strcmp(prefix, "MSG") == 0) {

		// print message
		printf("%s\n", message);

	}

	if (prefix != NULL)
		free(prefix);
	if (message != NULL)
		free(message);

	return (((EXIT_SUCCESS)));
}


char * cmd_argument(char * line) {
	return (((line+5)));
}

int process_client_request(int server_fd, int line_fd) {

	char * line = NULL;
	int result = get_delim(line_fd, &line, '\n');
	if (result == -1)
		err(1, "get_delim");

	if (line == strstr(line, "/cmd")) { // line begins with "/cmd"

		if (NULL != strstr(cmd_argument(line), " ")) {
			printf("Commands cannot contain spaces\n");
			return (((EXIT_SUCCESS)));
		}

		return (((send_command(server_fd, cmd_argument(line)))));

	} else if (strcmp(line, "/end") == 0) { // line begins with "/end"

		return (((send_end(server_fd))));
		exit(0);

	} else if (strcmp(line, "/ext") == 0) { // line begins with "/ext"

		return (((send_exit(server_fd))));
		exit(0);

	} else { // type of the dispatch is message

		return (((send_message(server_fd, line))));

	}

}
