/************************************************************************

   timers_1_scaling.c

 The purpose of this exercise is to determine (to within 1% or so) the
 proper scaling factor for each of the available time measurement methods
 on an Intel CPU in Linux, UNIX, or Mac OS X.

 To compile:

   gcc -O0 timers_1_scaling.c -lm -o 1_scaling

*/ 

#include <stdio.h>
#include <string.h>	    /* for memset() */
#include <sys/time.h>   /* for gettimeofday() */
#include <sys/times.h>  /* for gettimeofday() */
#include <sys/mman.h>   /* for mmap() */
#include <unistd.h>     /* for getpid() */

/* For calibrating gettimeofday() --> YOU WILL NEED TO CHANGE THIS */
#define TV_SCALING 1.5

/* For calibrating RDTSC --> YOU WILL NEED TO CHANGE THIS */
#define CLK_RATE 1.6e9

/* For calibrating get_seconds() --> YOU WILL NEED TO CHANGE THIS */
#define TIMES_CLK_TCK 50



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


int hit_return()
{
  char c;
  c = 0;
  while (((c = getc(stdin)) != EOF) && (c != '\n') && (c != '\r') ) {
    /* Hope they hit a key... */
  }
}

#define MEASURE_SECONDS 60.0

/*********************************************************************/
int main(int argc, char *argv[])
{
  int i;
  struct timeval tv_start, tv_stop; /* results from gettimeofday */
  tsc_counter tsc_start, tsc_stop;  /* results from RDTSC */
  double sec_start, sec_stop;       /* results from get_seconds */
  double target_meas = 0;
  double measurement = 0;
  double quasi_random = 0;
  int floor_meas, previous_floor;
  double previous_printed = 0;

  /* ====================================================================== */

  for(i=0;i<78;i++) printf("="); printf("\n");
  printf("We are going to try to measure out %.f seconds using gettimeofday().\n", MEASURE_SECONDS);
  printf("Hit RETURN or ENTER at the same moment that you start your stopwatch: ");
  hit_return();

  printf("\n");
  printf(" . . . now attempting to delay %.f seconds . . .\n", MEASURE_SECONDS);
  printf("\n");

  target_meas = MEASURE_SECONDS;
  measurement = 0.0; previous_floor = 0; previous_printed = 0;
  gettimeofday(&tv_start, NULL);
  while (measurement < target_meas) {
    for (i = 0; i < 100000; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
    }
    gettimeofday(&tv_stop, NULL);
    measurement = timeval_to_secs(&tv_stop) - timeval_to_secs(&tv_start);
    floor_meas = (int) measurement;
    if ((measurement > previous_printed*1.2) || (floor_meas > previous_floor)) {
      printf("  %f    \r", measurement); fflush(stdout);
      previous_floor = floor_meas; previous_printed = measurement;
    }
  }
  printf("    ////////////    \n");
  printf("    /// DONE ///    \n");
  printf("    ////////////    \n");
  printf("\n");

  printf("Did that actually take %.f seconds?\n", MEASURE_SECONDS);
  printf("  If it took less, decrease TV_SCALING\n");
  printf("  If it took more, increase TV_SCALING\n");
  printf("\n");
  printf("\n");
  printf("\n");


  /* ====================================================================== */

  for(i=0;i<78;i++) printf("="); printf("\n");
  printf("We are going to try to measure out %.f seconds using RDTSC.\n", MEASURE_SECONDS);
  printf("Hit RETURN or ENTER at the same moment that you start your stopwatch: ");
  hit_return();

  printf("\n");
  printf(" . . . now attempting to delay %.f seconds . . .\n", MEASURE_SECONDS);
  printf("\n");

  target_meas = MEASURE_SECONDS;
  measurement = 0.0; previous_floor = 0; previous_printed = 0;
  RDTSC(tsc_start);
  while (measurement < target_meas) {
    for (i = 0; i < 100000; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
    }
    RDTSC(tsc_stop);
    measurement = ((double)(tsc_stop.int64-tsc_start.int64)) / CLK_RATE;
    floor_meas = (int) measurement;
    if ((measurement > previous_printed*1.2) || (floor_meas > previous_floor)) {
      printf("  %f    \r", measurement); fflush(stdout);
      previous_floor = floor_meas; previous_printed = measurement;
    }
  }
  printf("    ////////////    \n");
  printf("    /// DONE ///    \n");
  printf("    ////////////    \n");
  printf("\n");

  printf("Did that actually take %.f seconds?\n", MEASURE_SECONDS);
  printf("  If it took less, increase CLK_RATE\n");
  printf("  If it took more, decrease CLK_RATE\n");
  printf("\n");
  printf("\n");
  printf("\n");


  /* ====================================================================== */

  for(i=0;i<78;i++) printf("="); printf("\n");
  printf("We are going to try to measure out %.f seconds using get_seconds.\n", MEASURE_SECONDS);
  printf("Hit RETURN or ENTER at the same moment that you start your stopwatch: ");
  hit_return();

  printf("\n");
  printf(" . . . now attempting to delay %.f seconds . . .\n", MEASURE_SECONDS);
  printf("\n");

  target_meas = MEASURE_SECONDS;
  measurement = 0.0; previous_floor = 0; previous_printed = 0;
  sec_start = get_seconds();
  while (measurement < target_meas) {
    for (i = 0; i < 100000; i++) {
      quasi_random = quasi_random*quasi_random - 1.923432;
    }
    sec_stop = get_seconds();
    measurement = sec_stop - sec_start;
    floor_meas = (int) measurement;
    if ((measurement > previous_printed*1.2) || (floor_meas > previous_floor)) {
      printf("  %f    \r", measurement); fflush(stdout);
      previous_floor = floor_meas; previous_printed = measurement;
    }
  }
  printf("    ////////////    \n");
  printf("    /// DONE ///    \n");
  printf("    ////////////    \n");
  printf("\n");

  printf("Did that actually take %.f seconds?\n", MEASURE_SECONDS);
  printf("  If it took less, increase TIMES_CLK_TCK\n");
  printf("  If it took more, decrease TIMES_CLK_TCK\n");
  printf("\n");
  printf("\n");
  printf("\n");


  /* ====================================================================== */

  printf("We took time calculating the number: %12.9f\n", quasi_random);
}
