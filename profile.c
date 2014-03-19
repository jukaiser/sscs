#ifndef NO_PROFILING
#include <time.h>
#include <stdio.h>

#include "profile.h"


static struct timespec start, stop;
static FILE *dbg;


void _prof_init ()

{
// return;
  dbg = fopen ("debug.profile", "w");
}


void _prof_enter ()

{
// return;
  fprintf (dbg, "[");
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
}


void _prof_leave (const char *msg)

{
// return;
  clock_gettime(CLOCK_MONOTONIC_RAW, &stop);

  struct timespec t;
  t.tv_nsec = stop.tv_nsec - start.tv_nsec;
  t.tv_sec  = stop.tv_sec - start.tv_sec;
  if (t.tv_nsec < 0)
    {
      // Borrow one sec
      t.tv_nsec += 1000000000;
      t.tv_sec--;
    }
  fprintf (dbg, " %ld.%09lds ] for %s\n", t.tv_sec, t.tv_nsec, msg);
  fflush (dbg);
}
#endif
