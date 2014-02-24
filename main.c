/* main body of Slow Salvo Construction Search program */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "guarded_ptr.h"
#include "reaction.h"
#include "database.h"
#include "queue.h"


main (int argc, char **argv)

{
  int nph;
  FILE *f;
  bullet *b;
  reaction *r;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <configfile>\n\nSearch for slow salvo constructions based on <configfile> given.\n", argv [0]);
      exit (1);
    }

  // Initialize all modules. (Starting with config, since config_load () might override variables used by other initialisations
  config_load (argv [1]);
  db_init ();
  queue_init ();
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN+1+MAXPERIOD, MAX_FIND);
  if (RELATIVE != 1)
    {
      fprintf (stderr, "Internal error: handling of non-relative lanes not implemented/undefined.\n");
      exit (3);
    }

  // load our bullet (lanes to use are defined with the bullet)
  b = db_bullet_load (BULLET);

  // Initialize the reaction handling module
  init_reactions ();

  // Preload all combinations of a candidate starting pattern and a bullet on any significant lane into our q.
  if (chdir (PATH) != 0) {perror (PATH); exit (2);}
  f = fopen (START, "r"); if (!f) {perror (START); exit (2);}
  while (!feof (f))
    {
      lab_init ();
      pat_load (f);

      // Is current pattern an oscillator?
      for (nph = 1; nph <= MAXPERIOD; nph++)
	{
	  pat_generate (&lab [nph-1], &lab [nph]);
	  if (pat_compare (&lab [0], &lab [nph]))
	    break;
	}

      // We only like still lifes and oscis with p <= MAXPERIOD
      if (nph > MAXPERIOD)
	continue;

      // The first nph generations of our lab [] contain the different phases of the current starting pattern.
      // extract them as targets.
      build_targets (0, nph);

      // build all possible reactions for these targets, queue them for later analysis and check them against our db.
      // NOTE: since we are handling starting patterns here, we don't have a "last lane used", yet.
      build_reactions (nph, b, true, -1, -1);
    }
  fclose (f);

  // main loop: take chepest reaction off the q, handle it (maybe queueing new reactions for later handling) and check if we could release some memory consuming objects.
  // Rinse and repeat, until queue runs empty (extremly unlikely!) or we get kicked by our user.
  int o_cost = 0;
  while (r = (reaction *) queue_grabfront ())
    {
if (o_cost < r->cost) queue_info (); o_cost = r->cost;
      handle (r);
// getchar ();
// puts ("\033[H\033[2J");

      if (!guard_unlink (r->g_tgt))
	{
	  pat_deallocate (((target*)r->g_tgt->ptr)->pat);
	  free (((target *)r->g_tgt->ptr)->pat);
	  free (r->g_tgt->ptr);
	  guard_dealloc (r->g_tgt);
	}
      free (r);
    }
}

