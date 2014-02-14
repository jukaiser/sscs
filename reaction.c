/* define and handle reactions */

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


static target **tgts;
static object *ships;


void build_reactions (int nph, bullet *b, int old_cost, int old_lane)

{
  guarded *gp;
  int i, j;
  reaction *r;

  for (i = 0; i < nph; i++)
    {
      int n = tgt_count_lanes (tgts [i], b);

      gp = guard_alloc (tgts [i]);

      if (LANES > 0 && LANES < n)
	n = LANES;
      for (j = 0; j < n; j++)
	{
	  r = malloc (sizeof (reaction));
	  if (!r) { perror ("malloc"); exit (2); }
	  if (old_lane < 0)
	    r->cost = 0;	// we're preloading!
	  else
	    r->cost = old_cost + COSTS [(nCOSTS + old_lane -j) % nCOSTS];
	  guard_link (gp);
	  r->g_tgt = gp;
	  r->bullet = b;
	  r->lane = j;

	  // Check our current reaction against db ... we don't need to follow it thru if it already handled completely.
	  if (!db_reaction_keep (r))
	    continue;

if (r->cost < 0) printf ("Huch? old_cost = %d, old_lane = %d, new_lane = %d {%d}\n", old_cost, old_lane, j, COSTS [(nCOSTS + old_lane -j) % nCOSTS]);
assert (r->cost >= 0);
	  if (!queue_insert (r->cost, r))
	    {
	      fprintf (stderr, "Q-insert failed (cost = %d)!\n", r->cost);
	      exit (1);
	    }
	}
    }
}


void build_targets (int start, int nph)

{
  int i;
  target *tgt;
  int left, right, top, bottom;

  // calculate common bbox of all phases of current target.
  top = MAXHEIGHT + 1;
  bottom = -1;
  left = MAXWIDTH + 1;
  right = -1;
  for (i = start; i < start+nph; i++)
    {
      if (lab [i].top    < top)    top    = lab [i].top;
      if (lab [i].bottom > bottom) bottom = lab [i].bottom;
      if (lab [i].left   < left)   left   = lab [i].left;
      if (lab [i].right  > right)  right  = lab [i].right;
    }

  // Iterate over all phases ...
  for (i = start; i < start+nph; i++)
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

      tgts [i-start] = tgt;
    }

  // sync them with our db.
  db_targets_keep (tgts, nph);
}


static void dies_at (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_dies);
}


static void explodes_at (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_unfinished);
}


static void emit (reaction *r, int gen, int offX, int offY, ROWID oId)

{
  db_store_emit (r->id, oId, offX, offY, gen);
}


static void fly_by (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_flyby);

return;
  if (SHIPMODE == 1 && r->lane + LANES < tgt_count_lanes (((target *)r->g_tgt->ptr), r->bullet))
    {
      reaction *new = malloc (sizeof (reaction));
      if (!new)
	{
	  perror ("reaction::fly_by - malloc");
	  exit (1);
	}

      new->id = 0;
      new->cost = r->cost;
      new->g_tgt = r->g_tgt;
      guard_link (new->g_tgt);
      new->bullet = r->bullet;
      new->lane = r->lane + LANES;

      printf ("Requeing as (%d, %p, %s, %d)\n",  new->cost, ((target *)new->g_tgt->ptr)->pat, new->bullet->name, new->lane);

      if (!queue_insert (new->cost, new))
	{
	  fprintf (stderr, "Q-insert failed (cost = %d)!\n", new->cost);
	  exit (1);
	}
    }
}



static void stabilizes (reaction *r, int i, int p)

{
  // The first nph generations of our lab [] starting with i contain the different phases of the resulting pattern
  // extract them as targets to tgts [] ...
  build_targets (i, p);

  // check with our db, retrieve the ROW-ids if the targets existed or store them if they are new.
  db_targets_keep (tgts, p);

  // remember our success in the db.
  target *old = (target *)r->g_tgt->ptr, *new = tgts [0];
  db_reaction_finish (r, new->id, new->left-old->left, new->top-old->top, i, dbrt_stable);

  // build all possible reactions for these targets, queue them for later analysis and check them against our db.
  // NOTE: since we are handling starting patterns here, we don't have a "last lane used", yet.
// printf ("TO DO: build_reactions (nph, -1);\n");
  build_reactions (p, r->bullet, r->cost, r->lane);	// FAKE!!
}


static void unstable (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_unfinished);
}


int search_ships (reaction *r, int gen)
// TO DO: improve detection so that only ships get counted that will *really* survive!

{
  found *f;
  int i, n;

  // Search for escaping space ships
  f = obj_search (gen, ships, &n);
  if (n < 1)
    return gen;

//printf ("%llu emitting %d ships before gen %d>>\n", r->id, n, gen);
//if (n > 1) pat_dump (&lab [gen]);

  // We know that at the current generation all objects in f have been around.
  // Find the point in time where the last ship appeared and remove them all.
  gen = obj_back_trace ();

  // Show the emitted ships
  for (i = 0; i < n; i++)
    emit (r, f [i].gen, f [i].offX, f [i].offY, f [i].obj->id);
//printf ("<< %llu done emitting for %d\n", r->id, gen);

  // maybe we are now smaller?
  pat_shrink_bbox (&lab [gen]);

  return gen;
}


void handle (reaction *r)

{
  int flyX, flyY, flyGen, i, p;
  target *tgt = r->g_tgt->ptr;

  // May be the the collision has been queued more then once.
  // And may *we* are not handling the cheapest of those.
  // Since we are queueing reactions in order of least costs we are able to check this.
  if (db_is_reaction_finished (r))
    return;

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
      // TO DO: that's not QUITE correct!
printf ("EMITTING BULLET!!\n");
      emit (r, keep, flyX, flyY, r->bullet->oId);
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

  // Maybe we didn't recognize a P2 because there is an (almost) emitted ship in our pattern?
  i = search_ships (r, MAXGEN+2);
  for (; i < MAXGEN; i++)
    pat_generate (&lab [i], &lab [i+1]);
  p = 1 + i - MAXGEN;

  // OK. check lab [MAXGEN] for p <= MAXPERIOD ...
  for (; p <= MAXPERIOD; p++)
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


void init_reactions (void)

{
  tgts = calloc (MAXPERIOD, sizeof (target *));
          
  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();
}
