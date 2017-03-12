
#include "common.h"
#include "proto.h"
#include "client_comm.h"
#include "thread_common.h"

void init_hints(struct addrinfo * hints_ptr) {

	bzero(hints_ptr, sizeof (*hints_ptr));
	hints_ptr->ai_family = AF_UNSPEC;
	hints_ptr->ai_socktype = SOCK_STREAM;

}

int get_connected_socket(char * server_address, unsigned short server_port) {

	int fd;
	char * server_port_string = malloc(6);
	struct addrinfo hints, * result, * addr_info;

	init_hints(&hints);

	snprintf(server_port_string, 6, "%d", server_port);

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

	return (fd);

}

void * lines_to_pipe(void * arg) {

	int * pipe = (int *) arg;
	int allocated, current;
	char * line;
	ssize_t wr_read, all_written;
	
	line = NULL;
	
	while (true) {
	   
		current = 0;
		allocated = 100;
		line = malloc(allocated);

		while (true) {

			// max length of a message, rest will be sent separately
			if (current >= 5000){
				break;
			}
			
			if (current+1 == allocated) {
				allocated += 100;
				line = realloc(line, allocated);
				if (line == NULL) {
					errx(1, "malloc");
				} 
			}
			
			line[current++] = getchar();

			if (line[current-1] == '\n') {
				printf("End of line\n");
				break;
			}

		}

		line[current] = '\0';

		all_written = 0;	   	
		while( all_written < current ){

			wr_read = write(*pipe, line+all_written, current);

			if (wr_read == -1) {
				err(1, "write");
			}

			all_written += wr_read;
		}

		free(line);

	}

	close(*pipe);

	return (NULL);
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
			} else if (result == -1)
				errx(1, "process_server_request");

		}

		// input from client
		if (fds[1].revents & POLLIN) {

			result = process_client_request(fds[0].fd, fds[1].fd);

			if (result == -1)
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
        init_pollfd_record(&fds[0], server_fd);

	// initialize pollfd for user-input
	init_pollfd_record(&fds[1], line_pipe[0]);

	while (true) {

		poll_cycle(&fds);

	}

	return (0);
}

int process_server_request(int fd) {

        char * message = NULL;

	enum dispatch_t disp_type = get_dispatch(fd, &message);

        switch(disp_type){
        case FAILURE:
          return (-1);
        case EOF_STREAM:
          return (EOF_IN_STREAM);
        case ERR:
          printf("ERR\n");
          break;
        case EXT:
          printf("EXT\n");
          break;
        case END:
          return (EOF_IN_STREAM);
        case CMD:
          send_end(fd);
          return (-1);
        case MSG:
          printf("%s\n", message);
        }

	if (message != NULL)
		free(message);

	return (0);

}

char * cmd_argument(char * line) {
	return (line+5);
}

int process_client_request(int server_fd, int line_fd) {

	char * line = NULL;
	int result = get_delim(line_fd, &line, '\n');
	if (result == -1)
		err(1, "get_delim");
	
	if (line == strstr(line, "/cmd")) { // line begins with "/cmd"

		if (NULL != strstr(cmd_argument(line), " ")) {
			printf("Commands cannot contain spaces\n");
			return (0);
		}

		return (send_command(server_fd, cmd_argument(line)));

	} else if (strcmp(line, "/end") == 0) { // line begins with "/end"

		return (send_end(server_fd));

	} else if (strcmp(line, "/ext") == 0) { // line begins with "/ext"

		return (send_exit(server_fd));

	} else { // type of the dispatch is message

		return (send_message(server_fd, line));

	}

	free(line);

}
