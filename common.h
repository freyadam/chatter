
#ifndef COMMON_H_
#define	COMMON_H_

#include "system_headers.h"

#define	DELIMITER ' '

#define	MAX_MSG_LEN 3000
#define	MAX_HEADER_LEN 50
struct thread_data {
		pthread_t id;
		int comm_fd;
		int priority_fd;
		char * name;
		struct thread_data * next;
};

struct new_thread_args {
		int comm_fd;
		int priority_fd;
		struct thread_data * data_ptr;
};

extern struct thread_data * thread_list;
extern pthread_mutex_t thr_list_mx;

struct comm_block {
	struct pollfd * fds;
	char ** names;
	int size;
};

#endif // COMMON_H_
