/* dummy test thingy */

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "queue.h"


typedef struct
  {
    int cost;
    int delta2parent;
    int i, j;
  } value;

main ()

{
  int i, j, delta;
  unsigned sum = 0, s2 = 0;
  FILE *rnd = fopen ("/dev/urandom", "r");
  value *v = malloc (sizeof (value)), *v2;

  queue_init ();

  if (!v || !queue_insert (0, v))
    {
      fprintf (stderr, "Jetzt schon?!\n");
      exit (1);
    }
  v->cost = 0;
  v->delta2parent = 0;
  v->i = -1;
  v->j = -1;

  for (i = 0; i < 10000000; i++)
    {
      v = queue_grabfront ();
      if (!v)
	{
	  fprintf (stderr, "Q is empty!\n");
	  exit (0);
	}
      // printf ("grabbed: [%d %d] -> %d (%d)\n", v->i, v->j, v->cost, v->delta2parent);
      sum += v->cost;

      for (j = 0; j < 10; j++)
	{
	  delta = 1+j; // + (fgetc (rnd) & 0x1F);
	  v2 = malloc (sizeof (value));
	  if (!v2)
	    {
	      fprintf (stderr, "Out of memory!\n");
	      exit (1);
	    }

	  v2->cost = v->cost + delta;
	  s2 += v2->cost;
	  v2->delta2parent = delta;
	  v2->i = i;
	  v2->j = j;
	  if (!queue_insert (v2->cost, v2))
	    {
	      fprintf (stderr, "queue_insert () failed!\n");
	      exit (1);
	    }

	  fflush (stdout);
	}
      free (v);
    }

  while (v = queue_grabfront ())
     sum += v->cost; //printf ("grabbed left over: [%d %d] -> %d (%d)\n", v->i, v->j, v->cost, v->delta2parent);

  printf ("Sum: %u\n", sum);
  printf ("Kontrolle: %u\n", s2);

  dump_qs ();
}

