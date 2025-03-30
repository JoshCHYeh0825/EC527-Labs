/****************************************************************************
 gcc -O1 test_doty.c -lrt -o test_doty
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

/* We want to test a range of work sizes using the quadratic formula */
#define A   1   /* coefficient of x^2 */
#define B   1   /* coefficient of x */
#define C   10  /* constant term */

#define NUM_TESTS 10   /* Number of different sizes to test */
#define OUTER_LOOPS 2000

#define CPNS 2.0    /* Cycles per nanosecond -- Adjust based on CPU frequency */
#define OPTIONS 3   /* Number of dot product implementations */

/* Dot product uses multiplication and addition */
#define IDENT 0.0
#define OP +

/* Type of data being used */
typedef double data_t;

/* Abstract data type for an array */
typedef struct {
  long int len;
  data_t *data;
} array_rec, *array_ptr;

/* Function Prototypes */
array_ptr new_array(long int len);
int init_array(array_ptr v, long int len);
void dot_product1(array_ptr v1, array_ptr v2, data_t *dest);
void dot_product2(array_ptr v1, array_ptr v2, data_t *dest);
void dot_product3(array_ptr v1, array_ptr v2, data_t *dest);
double interval(struct timespec start, struct timespec end);

/* Timing function */
double interval(struct timespec start, struct timespec end) {
  struct timespec temp;
  temp.tv_sec = end.tv_sec - start.tv_sec;
  temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  if (temp.tv_nsec < 0) {
    temp.tv_sec--;
    temp.tv_nsec += 1000000000;
  }
  return ((double)temp.tv_sec) + ((double)temp.tv_nsec) * 1.0e-9;
}

/* Ensures the CPU is running at full speed */
double wakeup_delay() {
  double meas = 0;
  int i, j;
  struct timespec time_start, time_stop;
  double quasi_random = 0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
  j = 100;
  while (meas < 1.0) {
    for (i = 1; i < j; i++) {
      quasi_random = quasi_random * quasi_random - 1.923432;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
    meas = interval(time_start, time_stop);
    j *= 2;
  }
  return quasi_random;
}

/* Main function */
int main(int argc, char *argv[]) {
  int OPTION;
  struct timespec time_start, time_stop;
  double time_stamp[OPTIONS][NUM_TESTS];
  double final_answer = 0;
  long int x, n, k, alloc_size;
  data_t *result;

  printf("Dot Product Testing\n");

  wakeup_delay();
  final_answer = wakeup_delay();

  x = NUM_TESTS - 1;
  alloc_size = A * x * x + B * x + C;

  /* Declare and initialize arrays */
  array_ptr v1 = new_array(alloc_size);
  array_ptr v2 = new_array(alloc_size);
  init_array(v1, alloc_size);
  init_array(v2, alloc_size);
  result = (data_t *) malloc(sizeof(data_t));

  printf("Testing %d dot product implementations,\n", OPTIONS);
  printf("  on vectors of %d sizes from %d to %ld\n", NUM_TESTS, C, alloc_size);

  /* Execute and time different implementations */
  OPTION = 0;
  printf("Testing dot_product1 (basic)\n");
  for (x = 0; x < NUM_TESTS && (n = A * x * x + B * x + C, n <= alloc_size); x++) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
    for (k = 0; k < OUTER_LOOPS; k++) {
      dot_product1(v1, v2, result);
      final_answer += *result;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
    time_stamp[OPTION][x] = interval(time_start, time_stop);
  }

  OPTION++;
  printf("Testing dot_product2 (unrolled by 2)\n");
  for (x = 0; x < NUM_TESTS && (n = A * x * x + B * x + C, n <= alloc_size); x++) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
    for (k = 0; k < OUTER_LOOPS; k++) {
      dot_product2(v1, v2, result);
      final_answer += *result;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
    time_stamp[OPTION][x] = interval(time_start, time_stop);
  }

  OPTION++;
  printf("Testing dot_product3 (unrolled by 4 with accumulators)\n");
  for (x = 0; x < NUM_TESTS && (n = A * x * x + B * x + C, n <= alloc_size); x++) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_start);
    for (k = 0; k < OUTER_LOOPS; k++) {
      dot_product3(v1, v2, result);
      final_answer += *result;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_stop);
    time_stamp[OPTION][x] = interval(time_start, time_stop);
  }

  /* Output times */
  printf("size, dot_product1, dot_product2, dot_product3\n");
  for (int i = 0; i < NUM_TESTS; i++) {
    printf("%ld, ", (A * i * i + B * i + C) * OUTER_LOOPS);
    for (int j = 0; j < OPTIONS; j++) {
      if (j != 0) printf(", ");
      printf("%ld", (long int)((double)(CPNS) * 1.0e9 * time_stamp[j][i]));
    }
    printf("\n");
  }

  printf("\nSum of all results: %g\n", final_answer);
  return 0;
}

/* Create a new array */
array_ptr new_array(long int len) {
  array_ptr result = (array_ptr) malloc(sizeof(array_rec));
  if (!result) return NULL;
  result->len = len;
  result->data = (data_t *) calloc(len, sizeof(data_t));
  return result;
}

/* Initialize an array */
int init_array(array_ptr v, long int len) {
  for (long int i = 0; i < len; i++) {
    v->data[i] = (data_t)(i % 100);
  }
  return 1;
}

/* Basic dot product */
void dot_product1(array_ptr v1, array_ptr v2, data_t *dest) {
  long int i;
  *dest = IDENT;
  for (i = 0; i < v1->len; i++) {
    *dest = *dest OP (v1->data[i] * v2->data[i]);
  }
}

/* Unrolled dot product (factor of 2) */
void dot_product2(array_ptr v1, array_ptr v2, data_t *dest) {
  long int i;
  data_t sum = IDENT;
  for (i = 0; i < v1->len - 1; i += 2) {
    sum += (v1->data[i] * v2->data[i]) + (v1->data[i + 1] * v2->data[i + 1]);
  }
  for (; i < v1->len; i++) {
    sum += v1->data[i] * v2->data[i];
  }
  *dest = sum;
}

/* Unrolled dot product (factor of 4 with accumulators) */
void dot_product3(array_ptr v1, array_ptr v2, data_t *dest) {
  long int i;
  data_t sum0 = IDENT, sum1 = IDENT, sum2 = IDENT, sum3 = IDENT;
  for (i = 0; i < v1->len - 3; i += 4) {
    sum0 += v1->data[i] * v2->data[i];
    sum1 += v1->data[i + 1] * v2->data[i + 1];
    sum2 += v1->data[i + 2] * v2->data[i + 2];
    sum3 += v1->data[i + 3] * v2->data[i + 3];
  }
  *dest = sum0 + sum1 + sum2 + sum3;
}
