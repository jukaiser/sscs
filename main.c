/* main body of Slow Salvo Construction Search program */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "database.h"
#include "queue.h"
#include "guided_ptr.h"


typedef struct
   {
     int     cost;
     guided *g_tgt;
     bullet *bullet;
     int     lane;
   } reaction;


target *tgt;
bullet *b;
reaction *r;
guided *gp;
object *ships;


static void dies_at (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): dies at %d\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, i);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void explodes_at (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): leaves lab [%d, %d]  at %d\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, lab[0].sizeX, lab [0].sizeY, i);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void emmit (reaction *r, int gen, int offX, int offY, pattern *what)

{
  printf ("Reaction (%d, %p, %s, %d): emmits space ship at %d\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, gen);
  pat_dump (&lab [0]);
  printf ("emitted (%d,%d):\n", offX, offY);
  pat_dump (what);
  printf ("\n");
}


static void fly_by (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): bullet '%s' flies by target!\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, r->bullet->name);
  pat_dump (&lab [0]);
  printf ("\n");
}


static void stabilizes (reaction *r, int i, int p)

{
  printf ("Reaction (%d, %p, %s, %d): stabilizes at %d [period %d]!\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, i, p);
  pat_dump (&lab [0]);
  pat_dump (&lab [i]);
  printf ("\n");
}


static void unstable (reaction *r, int i)

{
  printf ("Reaction (%d, %p, %s, %d): not stabilized after %d!\n", r->cost, ((target *)r->g_tgt->ptr)->pat, r->bullet->name, r->lane, i);
  pat_dump (&lab [0]);
  pat_dump (&lab [MAXGEN]);
  printf ("\n");
}


int search_ships (reaction *r, int gen)
// TO DO: back trace support!!

{
  found *f;
  int i, n;

  // Search for escaping space ships
  f = obj_search (gen, ships, &n);
  if (n < 1)
    return gen;

  // We know that at the current generation all objects in f have been around.
  // Find the point in time where the last ship appeared and remove them all.
  gen = obj_back_trace ();

  // Show the emitted ships
  for (i = 0; i < n; i++)
    emmit (r, f->gen, f->offX, f->offY, f->obj->pat);

  pat_dump (&lab [gen]);

  // maybe we are now smaller?
  pat_shrink_bbox (&lab [gen]);

  return gen;
}


static void handle (reaction *r)

{
  int flyX, flyY, flyGen, i, p;
  target *tgt = r->g_tgt->ptr;

  // Build collision defined by our reaction -> lab [0]
  lab_init ();
  tgt_collide (tgt, r->bullet, r->lane, &flyX, &flyY, &flyGen);

  // Fly-by detection ... generate until bullet would by definetly beyound target.
  for (i = 1; i <= flyGen; i++)
    if (!pat_generate (&lab [i-1], &lab [i]))
      {
	dies_at (r, i);
	return;
      }

  if (pat_match (&lab [flyGen], flyX, flyY, r->bullet->p))
    {
      int keep = flyGen-1;
      pat_remove (&lab [flyGen], flyX, flyY, r->bullet->p);
//       printf ("maybe fly-by (%d):\n", flyGen);
// if (W(&lab [flyGen]) <= 0) { printf ("WOOT?\n"); for (i = 0; i < flyGen; i++) pat_dump (&lab [i]); exit (0); }

      // Synchronise reaction with the uncollided target
      while (flyGen % tgt->nph)
	{
	  if (!pat_generate (&lab [flyGen], &lab [flyGen+1]))
	    {
	      dies_at (r, flyGen+1);
	      return;
	    }

	  flyGen++;
	}

      if (pat_match (&lab [flyGen], tgt->X, tgt->Y, tgt->pat))
	{
	  pat_remove (&lab [flyGen], tgt->X, tgt->Y, tgt->pat);
	  if (W(&lab [flyGen]) <= 0)
	    {
	      fly_by (r, i);
	      return;
	    }
	}

      // OK. The bullet escapes. But the target did not survive unharmed. Note the escaping ship and continue!
      emmit (r, keep, flyX, flyY, r->bullet->p);
    }

  // Here: no fly by. Follow the reaction until it stabilizes.
  for (i = flyGen+1; i <= MAXGEN+2; i++)
    {
      if (!pat_generate (&lab [i-1], &lab [i]))
	{
	  dies_at (r, i);
	  return;
	}
      else if (pat_touches_border (&lab [i]))
	{
	  i = search_ships (r, i);

	  if (!W(&lab [i]))
	    {
	      dies_at (r, i);
	      return;
	    }
	  else if (pat_touches_border (&lab [i]))
	    {
	      explodes_at (r, i);
	      return;
	    }
	}

      // Stabilized? Just check for P2 here, because that's cheap and anything P3 or beyond is *very* rare.
      if (i-2 >= 0 && pat_compare (&lab [i], &lab [i-2]))
	{
	  p = 2;
	  if (pat_compare (&lab [i-1], &lab [i-2]))
	    p = 1;

	  stabilizes (r, i-2, p);
	  return;
	}
    }

  // OK. lab [MAXGEN] is neither a still life nor a P2
  // Check for P>2 now.
  for (p = 3; p <= MAXPERIOD; p++)
    {
      // Welll. It COULD die even now ... so what.
      if (!pat_generate (&lab [MAXGEN + p-1], &lab [MAXGEN + p]))
	{
	  dies_at (r, MAXGEN + p);
	  return;
	}
      else if (pat_touches_border (&lab [MAXGEN + p]))
	{
	  explodes_at (r, MAXGEN + p);
	  return;
	}

      // Check if lab [] repeats with delta = p
      if (pat_compare (&lab [MAXGEN + p], &lab [MAXGEN]))
	{
	  // It really is a high period osci!
	  // Trace back to where it started!
	  for (i = MAXGEN; i >= p; i--)
	    if (!pat_compare (&lab [i], &lab [i+p]))
	      break;

	  stabilizes (r, i, p);
	  return;
	}
    }

  // Me didn't find nuffin'!
  unstable (r, MAXGEN);
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
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN+1+MAXPERIOD, MAX_FIND);

  // load our bullet (lanes to use are defined with the bullet)
  b = db_bullet_load (BULLET);

  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();

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
	  tgt = malloc (sizeof (target));
	  tgt->pat = pat_compact (&lab [i], NULL);
	  tgt->nph = nph;
	  tgt->top = top;
	  tgt->bottom = bottom;
	  tgt->left = left;
	  tgt->right = right;
	  tgt->X = lab [i].left;
	  tgt->Y = lab [i].top;

	  gp = guide_alloc (tgt);

	  int n = tgt_count_lanes (tgt, b);
	  if (LANES > 0 && LANES < n)
	    n = LANES;
	  for (j = 0; j < n; j++)
	    {
	      r = malloc (sizeof (reaction));
	      if (!r) { perror ("malloc"); exit (2); }
	      r->cost = 0;	// we're preloading!
	      guide_link (gp);
	      r->g_tgt = gp;
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
getchar ();
puts ("\033[H\033[2J");

      if (!guide_unlink (r->g_tgt))
	{
	  pat_deallocate (((target*)r->g_tgt->ptr)->pat);
	  free (((target *)r->g_tgt->ptr)->pat);
	  free (r->g_tgt->ptr);
	  guide_dealloc (r->g_tgt);
	}
      free (r);
    }
}

