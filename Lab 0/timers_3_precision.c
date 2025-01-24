/************************************************************************

  timers_3_precision.c

The purpose of this exercise is to estimate the precision of each of
three time measurement methods, by performing an identical task 20
times, each time using all three measurement methods. In an ideal
world, the three methods should agree since they are measuring the
same thing.

 To compile:

   gcc -O0 timers_3_precision.c -lm -o 3_precision

*/ 

#include <math.h>       /* for sqrt() */
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
  long long int i, j, k, limit, found_limit;
  long long int steps = 0;
  struct timeval tv_start, tv_stop;     /* used by gettimeofday */
  tsc_counter tsc_start, tsc_stop;      /* used by RDTSC */
  double sec_start, sec_stop;           /* used by get_seconds */
  double t_gtod, t_rdtsc, t_utime;
  double quasi_random = 0;

  double gtod_sumX2, gtod_sum_X, gtod_mean, gtod_var, gtod_stdev;
  double rdtsc_sumX2, rdtsc_sum_X, rdtsc_mean, rdtsc_var, rdtsc_stdev;
  double utime_sumX2, utime_sum_X, utime_mean, utime_var, utime_stdev;

  if ((TV_SCALING < 0) || (CLK_RATE < 0) || (TIMES_CLK_TCK < 0)) {
    printf("You have not yet modified this program as instructed in the\n"
           "comments! (where it says MODIFY)\n");
    exit(0);
  }

#define BIG_POW_10 1000000000
#define INNER_LOOP 100000

  /* First figure out how many "outer loops" to do. We want this to take about
     1 second per test. */

  printf("Finding out how many iterations takes about 1 second...\n");
  t_gtod = 0;
  for (limit=2; t_gtod < 1.0; limit = (limit * 3) / 2) {
    gettimeofday(&tv_start, NULL);
    for (i = 0; i <= limit; i++) {
      for (j = 0; j < INNER_LOOP; j++) {
        quasi_random = quasi_random*quasi_random - 1.923432;
        steps++;
      }
    }
    gettimeofday(&tv_stop, NULL);
    t_gtod = timeval_to_secs(&tv_stop) - timeval_to_secs(&tv_start);
    printf("...(%ld takes %.3f)...\r", limit*INNER_LOOP, t_gtod);
    found_limit = limit;
    fflush(stdout);
  }

  printf("\n\n");
  printf("It takes %d outer loops (and %ld total steps)\n",
                                       limit, (long long) limit * INNER_LOOP);
  printf("to be busy for %11.9f seconds\n", t_gtod);
  printf("\n");

  /* Now we will leave limit unchanged and repeat the i/j loops as before,
     and we'll do this 20 times. Each time, we'll measure the time using
     three different methods. */
  printf("Now performing 20 trials of the same experiment,\n");
  printf("        with three measurement methods:\n");
  printf("                   trial gettimeofday    RDTSC     get_seconds\n");

  gtod_sumX2 = gtod_sum_X = 0;
  rdtsc_sumX2 = rdtsc_sum_X = 0;
  utime_sumX2 = utime_sum_X = 0;
  for (k=0; k<20; k++) {
    gettimeofday(&tv_start, NULL);
    RDTSC(tsc_start);
    sec_start = get_seconds();
    for (i = 0; i <= found_limit; i++) {
      for (j = 0; j < INNER_LOOP; j++) {
        quasi_random = quasi_random*quasi_random - 1.923432;
        steps++;
      }
    }
    gettimeofday(&tv_stop, NULL);
    t_gtod = timeval_to_secs(&tv_stop) - timeval_to_secs(&tv_start);
    RDTSC(tsc_stop);
    t_rdtsc = (double)(tsc_stop.int64-tsc_start.int64)/CLK_RATE;
    sec_stop = get_seconds();
    t_utime = sec_stop - sec_start;
    printf("                     %2d  %11.9f  %11.9f  %11.9f\n",
                                               k, t_gtod, t_rdtsc, t_utime);

    gtod_sum_X += t_gtod;   gtod_sumX2  += t_gtod*t_gtod;
    rdtsc_sum_X += t_rdtsc; rdtsc_sumX2 += t_rdtsc*t_rdtsc;
    utime_sum_X += t_utime; utime_sumX2 += t_utime*t_utime;
  }
  printf("done\n");

  gtod_mean = gtod_sum_X / 20.0;
  gtod_var = (gtod_sumX2 / 20.0) - gtod_mean*gtod_mean;
  gtod_stdev = sqrt(gtod_var);

  rdtsc_mean = rdtsc_sum_X / 20.0;
  rdtsc_var = (rdtsc_sumX2 / 20.0) - rdtsc_mean*rdtsc_mean;
  rdtsc_stdev = sqrt(rdtsc_var);

  utime_mean = utime_sum_X / 20.0;
  utime_var = (utime_sumX2 / 20.0) - utime_mean*utime_mean;
  utime_stdev = sqrt(utime_var);

  printf("                gettimeofday      RDTSC     get_seconds\n");
  printf("           mean:   %6.4f        %6.4f        %6.4f\n",
                                     gtod_mean, rdtsc_mean, utime_mean);
  printf(" std. deviation:   %6.4f        %6.4f        %6.4f\n",
                                   gtod_stdev, rdtsc_stdev, utime_stdev);

  printf("We took time calculating the number: %12.9f\n", quasi_random);
}
