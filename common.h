
#ifndef COMMON_H_
#define	COMMON_H_

#include "system_headers.h"

#define	DELIMITER ' '

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

struct thread_data * thread_list;
pthread_mutex_t thr_list_mx;

#endif // COMMON_H_
