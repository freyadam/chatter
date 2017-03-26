
#include "common.h"
#include "accept.h"
#include "proto.h"
#include "users.h"

// how many authentication threads are running right now
int running_auth_threads = 0;
pthread_mutex_t auth_mx;

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

	freeaddrinfo(result);

	if (addr_info == NULL)
		err(1, "no valid gettaddrinfo result");

	if (listen(fd, SOMAXCONN) == -1)
		err(1, "listen");

	return (fd);

}

static void decrement_auth_count() {

	pthread_mutex_lock(&auth_mx);

	assert(running_auth_threads > 0);
	running_auth_threads--;

	pthread_mutex_unlock(&auth_mx);

}

static void * run_auth_thread(void * arg) {

	int client_fd = * ((int *) arg);
	free(arg);

	// authentication
	char * username = NULL;
	if (get_message(client_fd, &username) != 0) {
		close(client_fd);
		decrement_auth_count();
		return (NULL);
	}
	char * password = NULL;
	if (get_message(client_fd, &password) != 0) {
		free(username);
		close(client_fd);
		decrement_auth_count();
		return (NULL);
	}

	if (!user_present(username, password)) {
		send_message(client_fd, "Wrong username or password");
		free(username);
		free(password);
		close(client_fd);
		decrement_auth_count();
		return (NULL);
	}

	send_message(client_fd, "Connected.");

	pthread_mutex_lock(&thr_list_mx);

	// send username and file descriptor
	// of newly accepted client to the menu thread
	send_message(thread_list->comm_fd, username);
	send_message_f(thread_list->comm_fd, "%d", client_fd);

	pthread_mutex_unlock(&thr_list_mx);

	free(username);
	free(password);

	decrement_auth_count();
	return (NULL);
}

void accept_thread_cycle(int fd) {

	pthread_t auth_thread;
	int client_fd = accept(fd, NULL, NULL);
	int * thr_arg = malloc(sizeof (int));
	*thr_arg = client_fd;

	if (running_auth_threads < MAX_AUTH_THREADS) {
		pthread_mutex_lock(&auth_mx);
		running_auth_threads++;
		pthread_mutex_unlock(&auth_mx);

		if (pthread_create(&auth_thread, NULL,
		&run_auth_thread, (void *) thr_arg) != 0) {
			free(thr_arg);
		}
		pthread_detach(auth_thread);
	} else {
		free(thr_arg);
		send_message(client_fd, "Unable to connect right now.");
		close(client_fd);
	}

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

				pthread_mutex_init(&auth_mx, NULL);

	while (true) {

		accept_thread_cycle(fd);

	}

	return (NULL);
}

pthread_t create_accept_thread(int server_port) {

	pthread_t accept_thread;

	int * port_ptr = malloc(sizeof (int));
	*port_ptr = server_port;
	pthread_create(&accept_thread,
		NULL, &run_accept_thread, (void *)port_ptr);

	pthread_detach(accept_thread);

	return (accept_thread);
}
