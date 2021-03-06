/* main body of Slow Salvo Construction Search program */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"
#include "queue.h"
#include "profile.h"


static void Usage (const char *argv0)

{
  fprintf (stderr, "Usage: %s <configfile>\n\nSearch for slow salvo constructions based on <configfile> given.\n", argv0);
  exit (1);
}


main (int argc, char **argv)

{
  int nph;
  FILE *f;
  reaction *r;
  char *cfg_file;
  int  extend_old_search = -1;

  if (argc == 2 && argv [1][0] != '-')
    cfg_file = argv [1];
  else if (argc == 4 && strcmp (argv [1], "--extend-by") == 0)
    {
      extend_old_search = atoi (argv [2]);
      cfg_file = argv [3];
    }
  else
    Usage (argv [0]);

  // Initialize all modules. (Starting with config, since config_load () might override variables used by other initialisations
  config_load (cfg_file);
  _prof_init ();
  if (chdir (PATH) != 0)
    {
      perror (PATH);
      exit (2);
    }
  db_init ();
  queue_init ();
  MAXWIDTH = PRUNE_SX + MAXGEN+1+MAXPERIOD + 10;	// Make sure pattern evolution would never reach the border => simplify reliable ship detection
  MAXHEIGHT = PRUNE_SY + MAXGEN+1+MAXPERIOD + 10;	// Make sure pattern evolution would never reach the border => simplify reliable ship detection
printf ("Labsize: %dx%dx%d\n", MAXWIDTH, MAXHEIGHT, MAXGEN+1+MAXPERIOD);
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN+1+MAXPERIOD, MAX_FIND);
  if (RELATIVE != 1)
    {
      fprintf (stderr, "Internal error: handling of non-relative lanes not implemented/undefined.\n");
      exit (3);
    }

  // Initialize the reaction handling module
  init_reactions ();

  // Either we have to load a set of start targets and build reactions based on it, or we are extending an previous search ...
  if (extend_old_search > 0)
    {
      queue_restore_state (SAVEFILE);
      queue_extend_depth (extend_old_search);
    }
  else
    {
      // Preload all combinations of a candidate starting pattern and a bullet on any significant lane into our q.
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

	  // We only like still lifes and oscs with p <= MAXPERIOD
	  if (nph > MAXPERIOD)
	    continue;

	  // The first nph generations of our lab [] contain the different phases of the current starting pattern.
	  // extract them as targets.
	  build_targets (0, nph);
	  reaction_targets_keep (nph);

	  // build all possible reactions for these targets, queue them for later analysis and check them against our db.
	  // NOTE: since we are handling starting patterns here, we don't have a "last lane used", yet.
	  build_reactions (nph, 0, true, 0, 0);

	  // get rid of lingering targets ...
	  free_targets ();
	}
      fclose (f);
    }

  // main loop: take chepest reaction off the q, handle it (maybe queueing new reactions for later handling) and check if we could release some memory consuming objects.
  // Rinse and repeat, until queue runs empty (extremly unlikely!) or we get kicked by our user.
  unsigned o_cost = 0;
  while (r = (reaction *) queue_grabfront ())
    {
if (o_cost < r->cost) queue_info (); o_cost = r->cost;
      handle (r);
    }

  printf ("All reactions upto a cost of %d handled!\n", MAXCOST);

  // Keep all unhandled () reactions for later.
  queue_save_state (SAVEFILE);
}

