
#include "common.h"
#include "accept.h"
#include "proto.h"
#include "users.h"

void accept_signal_handler(int sig) {

	pthread_exit(NULL);

}

void init_hints(struct addrinfo * hints_ptr) {

	bzero(hints_ptr, sizeof (*hints_ptr));
	hints_ptr->ai_family = AF_UNSPEC;
	hints_ptr->ai_socktype = SOCK_STREAM;
	hints_ptr->ai_flags = AI_PASSIVE;

}

// return file descriptor on which the server
// 	is already listening (on port server_port)
int get_listening_socket(unsigned short server_port) {

	int fd, 	yes = 1;
	char server_port_string[6];
	struct addrinfo hints, * result, * addr_info;

	init_hints(&hints);

	snprintf(server_port_string, 6, "%d", server_port);

	if (getaddrinfo(NULL,
					server_port_string,
					&hints, &result) != 0)
		err(1, "getaddrinfo");

	for (addr_info = result;
		addr_info != NULL; addr_info = addr_info->ai_next) {

		if ((fd = socket(addr_info->ai_family,
		addr_info->ai_socktype, addr_info->ai_protocol)) == -1)
			continue;

		if (setsockopt(fd, SOL_SOCKET,
		SO_REUSEADDR, &yes, sizeof (int)) == -1)
			err(1, "setsockopt");


		if (bind(fd,
		addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
			close(fd);
			continue;
		}

		break;

	}

	if (addr_info == NULL)
		err(1, "no valid gettaddrinfo result");

	if (listen(fd, SOMAXCONN) == -1)
		err(1, "listen");

	return (fd);

}

void accept_thread_cycle(int fd) {

	int client_fd;
	char client_fd_str[6];

	client_fd = accept(fd, NULL, NULL);

	// authentication
	char * username = NULL;
	if (get_message(client_fd, &username) != 0) {
		close(client_fd);
		return;
	}
	char * password = NULL;
	if (get_message(client_fd, &password) != 0) {
		close(client_fd);
		return;
	}

	if (!user_present(username, password)) {
		send_message(client_fd, "Wrong username or password");
		close(client_fd);
		return;
	}

	send_message(client_fd, "Connected. ");

        free(username);
	free(password);

	// send username and file descriptor
	// of newly accepted client to the menu thread
	send_message(thread_list->comm_fd, username);

	snprintf(client_fd_str, 6, "%d", client_fd);
	send_message(thread_list->comm_fd, client_fd_str);

}

void set_signal_action() {

	// change signal mask to let SIGUSR1 through
	// set signal mask
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, 	SIGUSR1);
	if (pthread_sigmask(SIG_UNBLOCK, &signal_set, NULL) != 0)
		err(1, "pthread_sigmask");

	// set sigaction
	struct sigaction act;
	act. sa_handler = &accept_signal_handler;
	sigemptyset(&act. sa_mask);
	act. sa_flags = 0;
	if (sigaction(SIGUSR1, &act, NULL) == -1)
		err(1, "sigaction");

}

void * run_accept_thread(void * arg) {

	int fd, server_port;
	server_port = *((int *)arg);
	free(arg);

	set_signal_action();

	fd = get_listening_socket(server_port);

	while (true) {

		accept_thread_cycle(fd);

	}

	return (NULL);
}

pthread_t create_accept_thread(int server_port) {

	pthread_t accept_thread;

	int * port_ptr = malloc(sizeof(int));
	*port_ptr = server_port;
	pthread_create(&accept_thread,
		NULL, &run_accept_thread, (void *)port_ptr);

	return (accept_thread);
}
