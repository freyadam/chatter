
#include "system_headers.h"
#include "signal.h"



void run_signal_thread(){

  // catch incoming signals  
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  int received_signal;
  if( sigwait(&signal_set, &received_signal) != 0)
    err(1, "sigwait");

  assert( received_signal == SIGINT);

  // kill everyone else  
  struct thread_data * thr_ptr;  
  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next){
    write( thr_ptr->comm_fd, "END", 3);
  }
  
  printf("Exiting...\n");

  // join with other threads
  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next)
    pthread_join(thr_ptr->id, NULL); // pthread_join returns error but correctly returns result (???)

  exit(EXIT_SUCCESS);

}
