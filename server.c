
#include "common.h"
#include "proto.h"
#include "accept.h"
#include "signal.h"
#include "comm.h"
#include "menu.h"
#include "rooms.h"
#include "commands.h"
#include "users.h"

struct thread_data * thread_list;
pthread_mutex_t thr_list_mx;

void block_signals() {

	// set signal mask
	sigset_t signal_set;
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGKILL);
	sigdelset(&signal_set, SIGSTOP);

	if (pthread_sigmask(SIG_SETMASK, &signal_set, NULL) != 0)
		err(1, "pthread_sigmask");

}

void initiate_mutexes() {

	pthread_mutexattr_t mx_attr;
	if (pthread_mutexattr_init(&mx_attr) != 0)
		err(1, "pthread_mutexattr_init");
	if (pthread_mutexattr_settype(&mx_attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		err(1, "pthread_mutexattr_settype");

	if (pthread_mutex_init(&thr_list_mx, &mx_attr) != 0)
		errx(1, "pthread_mutex_init");
	if (pthread_mutex_init(&commands_mx, &mx_attr) != 0)
		errx(1, "pthread_mutex_init");
	if (pthread_mutex_init(&users_mx, &mx_attr) != 0)
		err(1, "pthread_mutex_init");

}

void run_server(int server_port) {

	initiate_mutexes();

	block_signals();

	// create menu thread
	create_menu_thread();

	// load rooms from file 'rooms'
	load_rooms(room_file);

	// load users
	load_users_from_file(user_file);
	list_users();

	// load commands
	printf("Commands:\n");
	load_commands(cmd_file);
	list_commands();

	pthread_t accept_thread = create_accept_thread(server_port);
	run_signal_thread(accept_thread);

	if (pthread_mutex_destroy(&thr_list_mx) != 0)
		errx(1, "pthread_mutex_destroy");

}

void print_server_settings(int server_port) {

	printf("PORT: %d\n", server_port);
	printf("CMD: %s\n", cmd_file);
	printf("USER: %s\n", user_file);
	printf("ROOM: %s\n", room_file);

}

int
main(int argc, char *argv[]) {

	// ----- INIT -----
	thread_list = NULL;
	int server_port = 4444;

	// default
	room_file = "./rooms";
	user_file = "./users";
	cmd_file = "./commands";

	int opt;

	// ----- GET OPTIONS -----
	while ((opt = getopt(argc, argv, "c:u:r:")) != -1)
		switch (opt) {
		case 'c':
			cmd_file = malloc((strlen(optarg) + 1));
			if (cmd_file == NULL)
				errx(1, "cmd_file malloc");
			strcpy(cmd_file, optarg); break;
		case 'u':
			user_file = malloc((strlen(optarg) + 1));
			if (user_file == NULL)
				errx(1, "user_file malloc");
			strcpy(user_file, optarg); break;
		case 'r':
			room_file = malloc((strlen(optarg) + 1));
			if (room_file == NULL)
				errx(1, "room_file malloc");
			strcpy(room_file, optarg); break;
		case '?':
			fprintf(stderr,
		"Usage: %s [-c command_list] [-u user_list] [-r room_list] \
		<port_number> \n",
		basename(argv[0]));
		exit(1);
		}

	char * port_arg = argv[optind];

	if (port_arg == NULL) {
		fprintf(stderr,
		"Usage: %s [-c command_list] [-u user_list] [-r room_list] \
		<port_number> \n",
		basename(argv[0]));
		exit(1);
	}

	server_port = strtol(port_arg, NULL, 10);
	if (server_port == 0 && errno == EINVAL)
		err(1, "strtol");
	if (server_port <= 1024)
		errx(1, "Port number has to be bigger than 1024.");

	// ----- PRINT SETTINGS -----
	print_server_settings(server_port);



	// ----- RUN -----
	run_server(server_port);

	return ((((EXIT_SUCCESS))));
}
