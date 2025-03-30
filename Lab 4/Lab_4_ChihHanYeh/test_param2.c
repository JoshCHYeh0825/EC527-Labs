/*************************************************************************

  gcc -pthread test_param2.c -o test_param2 -std=gnu99

 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10

/********************/
void *work(void *arg)
{
  int *data = (int *)arg; 
  int t_index = data[0];  
  int *t_array = data + 1;

  // Modify the thread’s assigned index in the array
 if (t_index >= 0 && t_index < NUM_THREADS){
  t_array[t_index] += t_index * 4;
  printf("Thread %d modified array[%d] to %d\n",
    t_index, t_index, t_array[t_index]);
  }

  free(arg);  // Free allocated memory
  pthread_exit(NULL);
}


/*************************************************************************/
int main(int argc, char *argv[])
{
    pthread_t id[NUM_THREADS];
    int t_array[NUM_THREADS];  // Create a shared array

    // Initialize array values
    for (int i = 0; i < NUM_THREADS; i++) {
        t_array[i] = i * 5;  
    }

    for (long t = 0; t < NUM_THREADS; ++t) {
        int *thread_data = malloc((NUM_THREADS + 1) * sizeof(int));
        if (thread_data == NULL) {
            printf("ERROR allocating memory\n");
            exit(1);
        }

        thread_data[0] = t;  // Store thread index
        for (int i = 0; i < NUM_THREADS; i++) {
            thread_data[i + 1] = t_array[i];  // Copy the array
        }

        if (pthread_create(&id[t], NULL, work, (void *)thread_data)) {
            printf("ERROR creating the thread\n");
            exit(19);
        }
    }

    // Wait for all threads
    for (long t = 0; t < NUM_THREADS; ++t) {
        pthread_join(id[t], NULL);
    }

    // Print final array
    printf("Final array: ");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("%d ", t_array[i]);
    }
    printf("\n");

    return 0;
} /* end main */
