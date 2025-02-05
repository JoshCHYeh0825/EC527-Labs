/*****************************************************************************
 To compile:

     gcc -O1 test_transpose.c -lrt -o test_transpose

 On some machines you might not need to link the realtime library. If
 you get an error like "library not found for -lrt", do not use the
 -lrt option.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define CPNS 3.0    /* Cycles per nanosecond -- Adjust to your computer,
                       for example a 3.2 GhZ GPU, this would be 3.2 */

/* We want to test a variety of test sizes. We will generate these
   using the quadratic formula:  A x^2 + B x + C                     */
#define A 8
#define B 16     
#define C 64   

#define NUM_TESTS 20   /* Number of different sizes to test */

#define OPTIONS 2
#define IDENT 0.0
#define OP +

typedef double data_t;

/* Create abstract data type for allocated array */
typedef struct {
  long int row_len;
  data_t *data;
} array_rec, *array_ptr;

/* Prototypes of functions in this program */
array_ptr new_array(long int row_len);
int set_row_length(array_ptr v, long int row_len);
long int get_row_length(array_ptr v);
int init_array(array_ptr v, long int row_len);
data_t *get_array_start(array_ptr v);
data_t combine2D(array_ptr v, int bsize);
data_t combine2D_rev(array_ptr v, int bsize);


/* -=-=-=-=- Time measurement by clock_gettime() -=-=-=-=- */
/*
  As described in the clock_gettime manpage (type "man clock_gettime" at the
  shell prompt), a "timespec" is a structure that looks like this:
 
        struct timespec {
          time_t   tv_sec;   // seconds
          long     tv_nsec;  // and nanoseconds
        };
 */

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

/* -=-=-=-=- End of time measurement declarations =-=-=-=- */

/* This routine "wastes" a little time to make sure the machine gets
   out of power-saving mode (800 MHz) and switches to normal speed. */
double wakeup_delay()
{
  double meas = 0; int i, j;
  struct timespec time_start, time_stop;
  double quasi_random = 0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
  j = 100;
  while (meas < 1.0) {
    for (i=1; i<j; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
    meas = interval(time_start, time_stop);
    j *= 2; 
  }
  return quasi_random;
}

/*****************************************************************************/
int main(int argc, char *argv[])
{
  int OPTION;
  struct timespec time_start, time_stop;
  double time_stamp[OPTIONS][NUM_TESTS];
  double final_answer;
  long int x, n, alloc_size;

  printf("2D Combine tests\n\n");

  final_answer = wakeup_delay();

  x = NUM_TESTS-1;
  alloc_size = (long)A*x*x + B*x + C;  // Larger matrices (e.g., 1000x� + 100)
  array_ptr v0 = new_array(alloc_size);
  init_array(v0, alloc_size);

  int block_sizes[] = {8, 16, 32, 64};
  int num_blocks = sizeof(block_sizes) / sizeof(block_sizes[0]);

  printf("row_len, block_size, combine2D, combine2D_rev\n");

  for (int b = 0; b < num_blocks; b++) {
    int bsize = block_sizes[b];

    // Reset time_stamp for each block size
    for (int i = 0; i < OPTIONS; i++) {
      for (int j = 0; j < NUM_TESTS; j++) {
        time_stamp[i][j] = 0.0;
      }
    }

    OPTION = 0;
    for (x = 0; x < NUM_TESTS && (n = A*x*x + B*x + C, n <= alloc_size); x++) {
      set_row_length(v0, n);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
      final_answer += combine2D(v0, bsize);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
      time_stamp[OPTION][x] = interval(time_start, time_stop);
    }

    OPTION++;
    for (x = 0; x < NUM_TESTS && (n = A*x*x + B*x + C, n <= alloc_size); x++) {
      set_row_length(v0, n);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
      final_answer += combine2D_rev(v0, bsize);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
      time_stamp[OPTION][x] = interval(time_start, time_stop);
    }

    // Print results for this block size
    for (int i = 0; i < NUM_TESTS; i++) {
      long row_len = A*i*i + B*i + C;
      printf("%ld, %d, ", row_len, bsize);
      for (int j = 0; j < OPTIONS; j++) {
        if (j != 0) printf(", ");
        printf("%ld", (long int)(CPNS * 1.0e9 * time_stamp[j][i]));
      }
      printf("\n");
    }
  }

  printf("Initial delay was calculating: %g \n", final_answer);
  return 0;
}

/* Rest of the code (new_array, set_row_length, etc.) remains unchanged */

/* end main */
/*********************************/

/* Create 2D array of specified size-per-dimension. The row_len parameter
   is both the width (row length or number of columns) and the height
   (column height or number of rows) */
array_ptr new_array(long int row_len)
{
  long int i;

  /* Allocate and declare header structure */
  array_ptr result = (array_ptr) malloc(sizeof(array_rec));
  if (!result) {
    /* Couldn't allocate storage */
    printf("\nCOULDN'T ALLOCATE %ld BYTES STORAGE\n", sizeof(result));
    exit(-1);
  }
  result->row_len = row_len;

  /* Allocate and declare array */
  if (row_len > 0) {
    data_t *data = (data_t *) calloc(row_len*row_len, sizeof(data_t));
    if (!data) {
      /* Couldn't allocate storage */
      free((void *) result);
      printf("\nCOULDN'T ALLOCATE %e BYTES STORAGE (row_len=%ld)\n",
                     (double)row_len * row_len * sizeof(data_t), row_len);
      exit(-1);
    }
    result->data = data;
  }
  else result->data = NULL;

  return result;
}

/* Set row-length of array (must already be allocated with new_array and
   size used when allocating must be big enough) */
int set_row_length(array_ptr v, long int row_len)
{
  v->row_len = row_len;
  return 1;
}

/* Return row length of array (which is also the number of columns) */
long int get_row_length(array_ptr v)
{
  return v->row_len;
}

/* initialize 2D array */
int init_array(array_ptr v, long int row_len)
{
  long int i;

  if (row_len > 0) {
    v->row_len = row_len;
    for (i = 0; i < row_len*row_len; i++) {
      v->data[i] = (data_t)(i);
    }
    return 1;
  }
  else return 0;
}

data_t *get_array_start(array_ptr v)
{
  return v->data;
}

/************************************/

/* Combine2D: Use operator "OP" (defined above as either + or *) to
   add or multiply all elements in the array. Accumulate the result
   in a local variable "accumulator", which becomes the return value. */
data_t combine2D(array_ptr v, int bsize)
{
  long int i, j, ii, jj;
  long int length = get_row_length(v);
  data_t *data = get_array_start(v);
  data_t accumulator = IDENT;

  for (ii = 0; ii < length; ii += bsize) {
    for (jj = 0; jj < length; jj += bsize) {
      for (i = ii; i < ii + bsize && i < length; i++) {
        for (j = jj; j < jj + bsize && j < length; j++) {
          accumulator = accumulator OP data[i * length + j];
        }
      }
    }
  }

  return accumulator;
}

/* Combine2D_rev:  Like combine2D but the loops are interchanged. */
data_t combine2D_rev(array_ptr v, int bsize)
{
  long int i, j, ii, jj;
  long int length = get_row_length(v);
  data_t *data = get_array_start(v);
  data_t accumulator = IDENT;

  for (jj = 0; jj < length; jj += bsize) {
    for (ii = 0; ii < length; ii += bsize) {
      for (j = jj; j < jj + bsize && j < length; j++){
        for (i = ii; i < ii + bsize && i < length; i++){
          accumulator = accumulator OP data[i * length + j];
        }
      }
    }
  }

  return accumulator;
}