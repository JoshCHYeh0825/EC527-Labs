/****************************************************************************

gcc -O1 -fopenmp test_SOR_omp.c -lrt -lm -o test_SOR_omp
OMP_NUM_THREADS=4 ./test_SOR_omp

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <omp.h>


#ifdef __APPLE__
/* Shim for Mac OS X (use at your own risk ;-) */
# include "apple_pthread_barrier.h"
#endif /* __APPLE__ */

#define CPNS 3.0    /* Cycles per nanosecond -- Adjust to your computer,
                       for example a 3.2 GhZ GPU, this would be 3.2 */

#define GHOST 2   /* 2 extra rows/columns for "ghost zone". */

#define A   8   /* coefficient of x^2 */
#define B   16  /* coefficient of x */
#define C   32  /* constant term */

#define THREADS 4

#define NUM_TESTS 5

/* A, B, and C needs to be a multiple of your BLOCK_SIZE,
   total array size will be (GHOST + Ax^2 + Bx + C) */

#define BLOCK_SIZE 8     // TO BE DETERMINED

#define OPTIONS 4

#define MINVAL   0.0
#define MAXVAL  10.0

#define TOL 0.00001
#define OMEGA 1.58       // TO BE DETERMINED

typedef double data_t;

typedef struct {
  long int rowlen;
  data_t *data;
} arr_rec, *arr_ptr;

/* Prototypes */
arr_ptr new_array(long int row_len);
int set_arr_rowlen(arr_ptr v, long int index);
long int get_arr_rowlen(arr_ptr v);
int init_array(arr_ptr v, long int row_len);
int init_array_rand(arr_ptr v, long int row_len);
int print_array(arr_ptr v);

void SOR_omp(arr_ptr v, int *iterations);

/* -=-=-=-=- Time measurement by clock_gettime() -=-=-=-=- */
/*
  As described in the clock_gettime manpage (type "man clock_gettime" at the
  shell prompt), a "timespec" is a structure that looks like this:
 
        struct timespec {
          time_t   tv_sec;   // seconds
          long     tv_nsec;  // and nanoseconds
        };
 */
void detect_threads_setting()
{
  long int ognt;
  char * env_ONT;

  /* Find out how many threads OpenMP wants to use */
  #pragma omp parallel
  {
    #pragma omp single
    {
      ognt = omp_get_num_threads();
    }
  }

  printf("OpenMP's default number of threads: %d\n", ognt);

  /* If the value is invalid, override with THREADS */
  if (ognt <= 0) {
    printf("Overriding with #define THREADS value %d\n", THREADS);
    ognt = THREADS;
  }

  omp_set_num_threads(ognt);

  /* Confirm the new thread count */
  #pragma omp parallel
  {
    #pragma omp single
    {
      ognt = omp_get_num_threads();
    }
  }
  
  printf("Using %d threads for OpenMP\n", ognt);
}

double interval(struct timespec start, struct timespec end)
{
  struct timespec temp;
  temp.tv_sec = end.tv_sec - start.tv_sec;
  temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  if (temp.tv_nsec < 0) {
    temp.tv_sec = temp.tv_sec - 1;
    temp.tv_nsec = temp.tv_nsec + 1000000000;
  }
  return (((double)temp.tv_sec) + ((double)temp.tv_nsec)*1.0e-9);
}
/*
     This method does not require adjusting a #define constant

  How to use this method:

      struct timespec time_start, time_stop;
      clock_gettime(CLOCK_REALTIME, &time_start);
      // DO SOMETHING THAT TAKES TIME
      clock_gettime(CLOCK_REALTIME, &time_stop);
      measurement = interval(time_start, time_stop);

 */


/* -=-=-=-=- End of time measurement declarations =-=-=-=- */

/*****************************************************************************/
int main(int argc, char *argv[])
{
  int OPTION;
  struct timespec time_start, time_stop;
  double time_stamp[OPTIONS][NUM_TESTS] = {0};  // Initialize all elements to 0
  int convergence[OPTIONS][NUM_TESTS] = {0};    // Initialize all elements to 0
  int iterations = 0;  // Use stack variable instead of malloc

  long int x, n;
  long int alloc_size;

  x = NUM_TESTS-1;
  alloc_size = GHOST + A*x*x + B*x + C;

  printf("SOR OMP \n");

  printf("OMEGA = %0.2f\n", OMEGA);

  detect_threads_setting();

  /* declare and initialize the array */
  arr_ptr v0 = new_array(alloc_size);

  OPTION = 0;
  printf("OPTION=%d (SOR_omp)\n", OPTION);
  for (x=0; x<NUM_TESTS && (n = A*x*x + B*x + C, n<=alloc_size); x++) {
    printf("  iter %d rowlen = %d\n", x, GHOST+n);
    init_array_rand(v0, GHOST+n);
    set_arr_rowlen(v0, GHOST+n);
    
    iterations = 0;  // Reset before each run
    printf("Before SOR_omp: iterations=%d\n", iterations);
    
    clock_gettime(CLOCK_REALTIME, &time_start);
    SOR_omp(v0, &iterations);
    clock_gettime(CLOCK_REALTIME, &time_stop);
    
    printf("After SOR_omp: iterations=%d\n", iterations);

    time_stamp[OPTION][x] = interval(time_start, time_stop);
    convergence[OPTION][x] = iterations;
  }

  printf("All times are in cycles (if CPNS is set correctly in code)\n");
  printf("\nsize, SOR_omp time, SOR_omp iters\n");
  for (x = 0; x < NUM_TESTS; x++) {
    printf("%4ld, %10.4g, %4d\n", 
        A * x * x + B * x + C,  
        CPNS * 1.0e9 * time_stamp[OPTION][x],  
        convergence[OPTION][x]);  
  }
  printf("\n");

  return 0;
}


/*********************************/

/* Create 2D array of specified length per dimension */
arr_ptr new_array(long int row_len)
{
  long int i;

  /* Allocate and declare header structure */
  arr_ptr result = (arr_ptr) malloc(sizeof(arr_rec));
  if (!result) {
    return NULL;  /* Couldn't allocate storage */
  }
  result->rowlen = row_len;

  /* Allocate and declare array */
  if (row_len > 0) {
    data_t *data = (data_t *) calloc(row_len*row_len, sizeof(data_t));
    if (!data) {
      free((void *) result);
      printf("\n COULDN'T ALLOCATE STORAGE \n", result->rowlen);
      return NULL;  /* Couldn't allocate storage */
    }
    result->data = data;
  }
  else result->data = NULL;

  return result;
}

/* Set row length of array */
int set_arr_rowlen(arr_ptr v, long int row_len)
{
  v->rowlen = row_len;
  return 1;
}

/* Return row length of array */
long int get_arr_rowlen(arr_ptr v)
{
  return v->rowlen;
}

/* initialize 2D array with incrementing values (0.0, 1.0, 2.0, 3.0, ...) */
int init_array(arr_ptr v, long int row_len)
{
  long int i;

  if (row_len > 0) {
    v->rowlen = row_len;
    for (i = 0; i < row_len*row_len; i++) {
      v->data[i] = (data_t)(i);
    }
    return 1;
  }
  else return 0;
}

/* initialize array with random data */
int init_array_rand(arr_ptr v, long int row_len)
{
  long int i;
  double fRand(double fMin, double fMax);

  /* Since we're comparing different algorithms (e.g. blocked, threaded
     with stripes, red/black, ...), it is more useful to have the same
     randomness for any given array size */
  srandom(row_len);
  if (row_len > 0) {
    v->rowlen = row_len;
    for (i = 0; i < row_len*row_len; i++) {
      v->data[i] = (data_t)(fRand((double)(MINVAL),(double)(MAXVAL)));
    }
    return 1;
  }
  else return 0;
}

/* print all elements of an array */
int print_array(arr_ptr v)
{
  long int i, j, row_len;

  row_len = v->rowlen;
  printf("row length = %ld\n", row_len);
  for (i = 0; i < row_len; i++) {
    for (j = 0; j < row_len; j++) {
      printf("%.4f ", (data_t)(v->data[i*row_len+j]));
    }
    printf("\n");
  }
}

data_t *get_array_start(arr_ptr v)
{
  return v->data;
}

double fRand(double fMin, double fMax)
{
  double f = (double)random() / RAND_MAX;
  return fMin + f * (fMax - fMin);
}

/************************************/

/* SOR */
void SOR_omp(arr_ptr v, int *iterations)
{
    long int i, j;
    long int rowlen = get_arr_rowlen(v);
    data_t *data = get_array_start(v);
    double change, total_change = 1.0e10;   /* start w/ something big */
    int iters = 0;
        
    while ((total_change / (double)(rowlen * rowlen)) > (double)TOL) {
        iters++;
        total_change = 0.0;
        
        #pragma omp parallel for private(i, j, change) reduction(+:total_change)
        for (i = 1; i < rowlen - 1; i++) {
            for (j = 1; j < rowlen - 1; j++) {
                change = data[i * rowlen + j] - 0.25 * (
                    data[(i - 1) * rowlen + j] +
                    data[(i + 1) * rowlen + j] +
                    data[i * rowlen + j + 1] +
                    data[i * rowlen + j - 1]
                );
                data[i * rowlen + j] -= change * OMEGA;
                total_change += fabs(change);
            }
        }

        if (fabs(data[(rowlen - 2) * rowlen + (rowlen - 2)]) > 10.0 * (MAXVAL - MINVAL)) {
            printf("SOR_omp: SUSPECT DIVERGENCE iter = %d\n", iters);
            break;
        }
    }

    *iterations = iters;
    printf("    SOR_omp() done after %d iters\n", iters);
}


