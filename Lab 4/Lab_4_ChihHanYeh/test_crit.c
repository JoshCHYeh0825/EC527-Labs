/*************************************************************************

  gcc -pthread test_crit.c -o test_crit -std=gnu99

 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10000

struct thread_data{
  int thread_id;
  int *balance;
  double qr;
};

double qr_total = 0;
pthread_mutex_t account_mutex;
pthread_mutex_t qr_mutex;


/********************/
void *PrintHello(void *threadarg)
{
  long int taskid;
  struct thread_data *my_data;
  int balance;
  double qr;

  my_data = (struct thread_data *) threadarg;
  taskid = my_data->thread_id;
  qr = my_data->qr;

  pthread_mutex_lock(&qr_mutex);
  qr_total += qr;
  pthread_mutex_unlock(&qr_mutex);

  /* Add up our quasi-random numbers, quasi-random delay, and add again */
  while (qr > 0.1) {
    qr = qr * 0.99;
  }

  pthread_mutex_lock(&qr_mutex);
  qr_total += qr;
  pthread_mutex_unlock(&qr_mutex);

  pthread_mutex_lock(&account_mutex);
  /* get the old balance */
  balance = *(my_data->balance);

  /* Modify balance */
  if (taskid % 2 == 0) {
    balance += 1;
  } else {
    balance -= 1;  
  }
  /* write new balance to global */
  *(my_data->balance) = balance;
  pthread_mutex_unlock(&account_mutex);


  /* printf(" It's me, thread #%ld! balance = %d\n", taskid, *balance); */

  pthread_exit(NULL);
}

/*************************************************************************/
int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  struct thread_data thread_data_array[NUM_THREADS];
  int rc;
  int account = 1000;
  double quasi_random = 0;
  pthread_mutex_init(&account_mutex, NULL);
  pthread_mutex_init(&qr_mutex, NULL);


  printf("Hello test_crit.c\n");

  for (long t = 0; t < NUM_THREADS; t++) {
    quasi_random = quasi_random*quasi_random - 1.923432;
    thread_data_array[t].thread_id = t+1;
    thread_data_array[t].balance = &account;
    thread_data_array[t].qr = quasi_random + 2.0;
    /* printf("In main:  creating thread %ld\n", t+1); */
    rc = pthread_create(&threads[t], NULL, PrintHello,
                                           (void*) &thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  for (long t = 0; t < NUM_THREADS; t++) {
    if (pthread_join(threads[t],NULL)){
      exit(19);
    }
  }
  
  pthread_mutex_destroy(&account_mutex);
  pthread_mutex_destroy(&qr_mutex);

  printf(" MAIN --> final balance = %d\n", account); 
  printf("          qr_total = %f\n", qr_total); 

  pthread_exit(NULL);
} /* end main */
