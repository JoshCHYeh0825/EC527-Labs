/************************************************************************

  timers_2_resolution.c

The purpose of this exercise is to estimate the resolution that we can
get from each of the available time measurement methods on an Intel
CPU in Linux, UNIX, or Mac OS X.

 To compile:

   gcc -O0 timers_2_resolution.c -lm -o 2_resolution

*/ 

#include <stdio.h>
#include <stdlib.h>     /* for exit() */
#include <string.h>	    /* for memset() */
#include <sys/time.h>   /* for gettimeofday() */
#include <sys/times.h>  /* for gettimeofday() */
#include <sys/mman.h>   /* for mmap() */
#include <unistd.h>     /* for getpid() */


/* MODIFY the following three lines to use the values you determined
   when you worked on timers_1_scaling.c */
#define TV_SCALING -1.0
#define CLK_RATE -1.0e9
#define TIMES_CLK_TCK -1



/* -=-=-=-=-= Time measurement by gettimeofday() -=-=-=-=- */

/* Turn the result from gettimeofday into a single number in units of seconds.
     gettimeofday is measuring the time since the UNIX epoch in 1970, which
   means we need a lot of digits to be able to measure intervals with
   microsecond resolution. Therefore we do the computation in "long double"
   which is good for about 19 decimal digits. */
#define timeval_to_secs(p_tv) \
   (   ( ((long double) (p_tv)->tv_sec) \
       + ((long double) (p_tv)->tv_usec) * 1.0e-6 ) \
   * TV_SCALING )
/*
     TV_SCALING constant is defined above

  How to use this method:

      struct timeval tv_start, tv_stop;
      gettimeofday(&tv_start, NULL);
      // DO SOMETHING THAT TAKES TIME
      gettimeofday(&tv_stop, NULL);
      measurement = timeval_to_secs(&tv_stop) - timeval_to_secs(&tv_start);
 */



/* -=-=-=-=-=-=-= Time measurement by RDTSC =-=-=-=-=-=-=- */

/* This union struct is for calling RDTSC and reinterpreting its two
   32-bit integer results as a single 64-bit integer                  */
typedef union {
  unsigned long long int64;
  struct {unsigned int lo, hi;} int32;
} tsc_counter;

/* We define RDTSC using inline assembly language instruction rdtsc */
#define RDTSC(cpu_c)              \
  __asm__ __volatile__ ("rdtsc" : \
  "=a" ((cpu_c).int32.lo),        \
  "=d"((cpu_c).int32.hi))
/*
     also uses CLK_RATE constant which is defined above

  How to use this method:

      tsc_counter tsc_start, tsc_stop;
      RDTSC(tsc_start);
      // DO SOMETHING THAT TAKES TIME
      RDTSC(tsc_stop);
      measurement = ((double)(tsc_stop.int64-tsc_start.int64)) / CLK_RATE;

 */



/* -=-=-=-=-=-=- Time measurement by times() -=-=-=-=-=-=- */

double get_seconds() { 	/* routine to read time */
    struct tms rusage;
    times(&rusage);	/* UNIX utility: time in clock ticks */
    return (double)(rusage.tms_utime)/(double)(TIMES_CLK_TCK);
}
/*
     TIMES_CLK_TCK constant is defined above

  How to use this method:

      double sec_start, sec_stop;
      sec_start = get_seconds();
      // DO SOMETHING THAT TAKES TIME
      sec_stop = get_seconds();
      measurement = sec_stop - sec_start;

 */


/* -=-=-=-=- End of time measurement declarations =-=-=-=- */


/*********************************************************************/
int main(int argc, char *argv[])
{
  long long int i, j, k, limit;
  long long int steps = 0;
  double sec_start, sec_stop, meas;
  struct timeval tv_start, tv_stop;   /* used by gettimeofday */
  tsc_counter tsc_start, tsc_stop;    /* used by RDTSC */
  double quasi_random = 0;

#define LONGEST_LOOP 1000000000

  if ((TV_SCALING < 0) || (CLK_RATE < 0) || (TIMES_CLK_TCK < 0)) {
    printf("You have not yet modified this program as instructed in the\n"
           "comments! (where it says MODIFY)\n");
    exit(0);
  }

  /* Test gettimeofday(). We try many loop sizes starting with something that
     will take about 1 second and halving many times. */

  printf("Using gettimeofday: \n");

  j = 0;
  for (limit = LONGEST_LOOP; limit >= 1; limit /= 2) {
    gettimeofday(&tv_start, NULL);
    for (i = 0; i <= limit; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
      steps++;
    }
    gettimeofday(&tv_stop, NULL);
    meas = timeval_to_secs(&tv_stop) - timeval_to_secs(&tv_start);
    printf(" looped %10ld times, measured %11.9f sec\n", limit+1, meas);
  }

  printf("gettimeofday tests done, %lld steps total\n", steps);
  printf("\n");


  /* Test RDTSC().  Again we try many loop sizes. */

  printf("Using RDTSC: \n");

  j = 0;
  for (limit = LONGEST_LOOP; limit >= 1; limit /= 2) {
    RDTSC(tsc_start);
    for (i = 0; i <= limit; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
      steps++;
    }
    RDTSC(tsc_stop);
    meas = (double)(tsc_stop.int64-tsc_start.int64)/CLK_RATE;
    printf(" looped %10ld times, measured %11.9f sec (%lld cycles)\n",
                               limit+1, meas, tsc_stop.int64-tsc_start.int64);
  }

  printf("RDTSC tests done, %lld steps total\n", steps);
  printf("\n");


  /* Test times() through get_seconds(). As before, we try many loop sizes. */

  printf("Using times(): \n");

  for (limit = LONGEST_LOOP; limit >= 1; limit /= 2) {
    sec_start = get_seconds();
    for (i = 0; i <= limit; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
      steps++;
    }
    sec_stop = get_seconds();
    meas = sec_stop - sec_start;
    printf(" looped %10ld times, measured %11.8f seconds\n", limit+1, meas);
  }

  printf("times() tests done, total steps = %lld \n", steps);
  printf("\n");

  printf("We took time calculating the number: %12.9f\n", quasi_random);
}
