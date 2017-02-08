
#include "system_headers.h"
#include "signal.h"

void run_signal_thread(pthread_t accept_thread){

  // catch incoming signals  
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  int received_signal;
  if( sigwait(&signal_set, &received_signal) != 0)
    err(1, "sigwait");

  assert( received_signal == SIGINT);

  // kill accepting thread
  pthread_kill(accept_thread, SIGUSR1);

  // kill everyone else  
  if( pthread_mutex_lock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_lock");

  struct thread_data * thr_ptr;  
  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next){
    write( thr_ptr->priority_fd, "END", 3);
  }
  if( pthread_mutex_lock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_lock");
  
  printf("Exiting...\n");

  // join with other threads
  if( pthread_mutex_lock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_lock");

  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next){
    pthread_join(thr_ptr->id, NULL); // pthread_join returns error but correctly returns result (???)
  }
  
  if( pthread_mutex_lock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_lock");  

}
