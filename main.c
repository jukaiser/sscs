/* main body of Slow Salvo Construction Search program */

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "pattern.h"
#include "database.h"
#include "queue.h"
#include "guided_ptr.h"


typedef struct
   {
     int     cost;
     guided *target;
     bullet *bullet;
     int     lane;
   } reaction;


pattern *target, *temp;
bullet *b;
reaction *r;
guided *gp;


static void dies_at (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): dies at %d\n", r->cost, r->target->ptr, r->bullet->name, r->lane, i);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void emmit (reaction *r, int flyGen, int flyX, int flyY, pattern *what)

{
  printf ("Reaction (%d, %p, %s, %d): emmits space ship at %d\n", r->cost, r->target->ptr, r->bullet->name, r->lane, flyGen);
  pat_dump (&lab [0]);
  printf ("emitted (%d,%d):\n", flyX, flyY);
  pat_dump (what);
  printf ("\n");
}


static void fly_by (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): bullet '%s' flies by target!\n", r->cost, r->target->ptr, r->bullet->name, r->lane, r->bullet->name);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void stabilizes (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): stabilizes at %d!\n", r->cost, r->target->ptr, r->bullet->name, r->lane, i);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void unstable (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): not stabilized after %d!\n", r->cost, r->target->ptr, r->bullet->name, r->lane, i);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void handle (reaction *r)

{
  int flyX, flyY, flyGen, i;

  // Build collision defined by our reaction -> lab [0]
  lab_init ();
  pat_collide (r->target->ptr, r->bullet, r->lane, &flyX, &flyY, &flyGen);

  // Fly-by detection ... generate until bullet would by definetly beyound target.
  for (i = 1; i <= flyGen; i++)
    if (!pat_generate (&lab [i-1], &lab [i]))
      {
	dies_at (r, i);
	return;
      }
  if (pat_match (&lab [flyGen], flyX, flyY, r->bullet->p))
    {
      pat_remove (&lab [flyGen], flyX, flyY, r->bullet->p);
//       printf ("maybe fly-by (%d):\n", flyGen);
// if (W(&lab [flyGen]) <= 0) { printf ("WOOT?\n"); for (i = 0; i < flyGen; i++) pat_dump (&lab [i]); exit (0); }

      while (flyGen % MAXPERIOD)
	{
	  if (!pat_generate (&lab [flyGen], &lab [flyGen+1]))
	    {
	      emmit (r, flyGen, flyX, flyY, r->bullet->p);
	      dies_at (r, i);
	      return;
	    }
	  flyGen++;
	}

      pat_copy (temp, (lab [0].sizeX-W((pattern *)r->target->ptr))/2, (lab [0].sizeY-H((pattern *)r->target->ptr))/2, (pattern *)r->target->ptr);
      pat_shrink_bbox (temp);
      if (pat_match (&lab [flyGen], 0, 0, temp))
	{
	  pat_remove (&lab [flyGen], 0, 0, temp);
	  if (W(&lab [flyGen]) <= 0)
	    {
	      fly_by (r, i);
	      return;
	    }
	}

      emmit (r, flyGen, flyX, flyY, r->bullet->p);
    }

  // Here: no fly by. Follow the reaction until it stabilizes.
  for (i = flyGen; i < MAXGEN + MAXPERIOD-2; i++)
    {
      if (!pat_generate (&lab [i], &lab [i+1]))
	{
	  dies_at (r, i);
	  return;
	}

      // Stabilized?
      if (i+1-MAXPERIOD >= 0 && pat_compare (&lab [i+1], &lab [i+1-MAXPERIOD]))
	{
	  stabilizes (r, i+1-MAXPERIOD);
	  return;
	}
    }

  unstable (r, i);
}


main (int argc, char **argv)

{
  int i, j, nph, left, right, top, bottom;
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <configfile>\n\nSearch for slow salvo constructions based on <configfile> given.\n", argv [0]);
      exit (1);
    }

  // Initialize all modules. (Starting with config, since config_load () might override variables used by other initialisations
  config_load (argv [1]);
  db_init ();
  queue_init ();
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN+MAXPERIOD-1);
  temp = pat_allocate (NULL, MAXWIDTH, MAXHEIGHT);

  // load our bullet (lanes to use are defined with the bullet)
  b = db_bullet_load (BULLET);

  // Preload all combinations of a candidate starting pattern and a bullet on any significant lane into our q.
  if (chdir (PATH) != 0) {perror (PATH); exit (2);}
  f = fopen (START, "r"); if (!f) {perror (START); exit (2);}
  while (!feof (f))
    {
      lab_init ();
      pat_load (f);

      // Is current pattern an oscillator?
      // WARNING: garbage in -> garbage out! if pattern is osc with period > MAXPERIOD this fails!
      // WARNING: garbage in -> garbage out! if pattern is empty or dies out or is unstable ... FAIL!
      nph = 1;
      for (i = 1; i < MAXPERIOD; i++)
	{
	  pat_generate (&lab [i-1], &lab [i]);
	  if (pat_compare (&lab [0], &lab [i]))
	    break;
	  else
	    nph++;
	}

      // calculate common bbox of all phases of current target.
      top = MAXHEIGHT + 1;
      bottom = -1;
      left = MAXWIDTH + 1;
      right = -1;
      for (i = 0; i < nph; i++)
	{
	  if (lab [i].top    < top)    top    = lab [i].top;
	  if (lab [i].bottom > bottom) bottom = lab [i].bottom;
	  if (lab [i].left   < left)   left   = lab [i].left;
	  if (lab [i].right  > right)  right  = lab [i].right;
	}

      // Iterate over all phases ...
      for (i = 0; i < nph; i++)
	{
	  target = pat_allocate (NULL, right-left+1, bottom-top+1);
	  pat_fill (target, DEAD);

	  pat_add (target, -left, -top, &lab [i]);
	  gp = guide_alloc (target);

	  for (j = 0; j < pat_count_lanes (target, b); j++)
	    {
	      r = malloc (sizeof (reaction));
	      if (!r) { perror ("malloc"); exit (2); }
	      r->cost = 0;	// we're preloading!
	      guide_link (gp);
	      r->target = gp;
	      r->bullet = b;
	      r->lane = j;

	      if (!queue_insert (r->cost, r))
		{
		  fprintf (stderr, "Q-insert failed!\n");
		  exit (1);
		}
	    }
	  }
    }
  fclose (f);

  while (r = (reaction *) queue_grabfront ())
    {
      handle (r);

      if (!guide_unlink (r->target))
	{
	  pat_deallocate (r->target->ptr);
	  free (r->target->ptr);
	  guide_dealloc (r->target);
	}

      // pat_dump (&lab [0]);
    }
}

