/* define and handle reactions */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"
#include "queue.h"


static target **tgts;
static object *ships;


int cost_for (int old_lane, int new_lane)

{
  int d = new_lane - old_lane;

  // C operator % does not what we want, if LHS < 0
  while (d < 0)
    d += nCOSTS;

  return COSTS [d % nCOSTS];
}


void reaction_targets_keep (int nph)

{
  int i;

  // sync all we know about tgts with the DB.
  for (i = 0; i < nph; i++)
    {
      char *rle = pat_rle (tgts [i]->pat);
      if (!db_target_lookup (tgts [i], rle))
        db_target_store (tgts [i], rle);
    }

  if (nph > 1)
    for (i = 0; i < nph; i++)
      db_target_link (i ? tgts [i-1]->id : tgts [nph-1]->id, tgts [i]->id);
}


void free_target (target *t)

{
  pat_deallocate (t->pat);
  free (t->pat);
}


void free_targets (void)

{
  int i, nph;

  assert (tgts [0]);
  assert (tgts [0]->nph > 0 && tgts [0]->nph <= MAXPERIOD);
  nph = tgts [0]->nph;
  for (i = 0; i < nph; i++)
    {
      pat_deallocate (tgts [i]->pat);
      free (tgts [i]->pat);
      free (tgts [i]);
      tgts [i] = NULL;
    }
}


void build_reactions (int nph, int b, bool preload, unsigned old_cost, int old_lane)

{
  int i, j, cost;
  uint8_t delta;

  for (i = 0; i < nph; i++)
    {
      int n = tgt_count_lanes (tgts [i], b);

      if (LANES > 0 && LANES < n)
	n = LANES;
      for (j = 0; j < n; j++)
	{
	  if (preload)
	    {
	      // we're preloading!
	      cost = 0;
	      delta = INT8_C (0);
	    }
	  else
	    {
	      int d = cost_for (old_lane, j);
	      assert (d > 0 && d < UINT8_MAX);
	      delta = (uint8_t) d;
	      cost =  (unsigned) old_cost + (unsigned) delta;
	    }

	  // Check our current reaction against db ... we don't need to follow it thru if it already handled completely.
	  if (!db_is_reaction_finished (tgts [i]->id, b, j))
	    queue_insert (cost, tgts [i]->id, b, (uint8_t) j, delta);
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
}


static void dies_at (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_dies);
}


static void explodes_at (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_unfinished);
printf ("FATAL: should no longer happen! [%llu, %d]\n!", r->rId, i);
assert (0);
}


static void emit (reaction *r, int gen, int offX, int offY, ROWID oId)

{
  // printf ("calling db_store_emit (%llu, %llu, %d, %d, %d)\n", r->rId, oId, offX, offY, gen);
  db_store_emit (r->rId, oId, offX, offY, gen);
}


static bool fly_by (reaction *r, target *tgt, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_flyby);

  if (SHIPMODE == 1 && r->lane + LANES < tgt_count_lanes (tgt, r->b))
    {
      r->lane += LANES;

      assert (r->lane <= UINT8_MAX);
      r->rId = 0;

      return !db_is_reaction_finished (r->tId, r->b, r->lane);
    }

  return false;
}


static void prune (reaction *r)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, 0, dbrt_pruned);
}


static void stabilizes (reaction *r, target *old, int i, int p)

{
  // The first nph generations of our lab [] starting with i contain the different phases of the resulting pattern
  // extract them as targets to tgts [] ...
  build_targets (i, p);

  // Tho the pattern did stabilize it might still be to big for us to follow it further.
  // TO DO: CHECK for other prune-conditions here!
  if (W(tgts [0]) > PRUNE_SX || H(tgts [0]) > PRUNE_SY)
    {
      prune (r);
      free_targets ();
      return;
    }

  // check with our db, retrieve the ROW-ids if the targets existed or store them if they are new.
  reaction_targets_keep (p);

  // remember our success in the db.
  target *new = tgts [0];
  db_reaction_finish (r, new->id, new->left-old->left, new->top-old->top, i, dbrt_stable);

  // build all possible reactions for these targets, queue them for later analysis and check them against our db.
  // NOTE: since we are handling starting patterns here, we don't have a "last lane used", yet.
  build_reactions (p, r->b, false, r->cost, r->lane + tgt_adjust_lane (r->b, old, new));

  // don't let them linger around any longer then they are needed!
  free_targets ();
}


static void unstable (reaction *r, int i)

{
  db_reaction_finish (r, (ROWID)0, 0, 0, i, dbrt_unfinished);
}


static int search_ships (reaction *r, target *tgt, int gen)

{
  found *f;
  int i, n;

  // Search for escaping space ships
  f = obj_search (gen, ships, &n);
  if (!f)
    {
      // obj_search () found something we do NOT like to find. Discard this reaction!
      db_reaction_emits (r->rId);
      prune (r);
      return -1;
    }
  if (n < 1)
    return gen;

  // We know that at the current generation all objects in f have been around.
  // Find the point in time where the last ship appeared and remove them all.
  gen = obj_back_trace ();

  // Store the emitted ships
  // TO DO: recalculate the offsets. They should be adjusted for the position of the orignal pattern.
  //        f [i].offX/offY are absolute positions within lab [gen]
  for (i = 0; i < n; i++)
    emit (r, f [i].gen, f [i].offX-tgt->X, f [i].offY-tgt->Y, f [i].obj->id);

  return gen;
}


void handle (reaction *r)

{
  int flyX, flyY, flyGen, i, p;
  target tgt;
  bool re_fly;

assert (r);
assert (!r->rId);

  // if we detect a fly-by condition we might want to immediately rerun the reaction on a higher lane ...
  re_fly = false;
  do
    {
      // Maybe the collision has been queued more then once.
      // And maybe *we* are not handling the cheapest of those.
      // Since we are queueing reactions in order of least cost we are able to check this.
      if (!db_reaction_keep (r))
	return;

      // Fetch our target from the datebase
      // BUT: only if this is not a fly-by-rerun
      if (!re_fly)
	db_target_fetch (r, &tgt);

      // reset the "fly by detected" flag.
      re_fly = false;

      // Build collision defined by our reaction -> lab [0]
      lab_init ();
      tgt_collide (&tgt, &bullets [r->b], r->lane, &flyX, &flyY, &flyGen);

      // Generate until MAXGEN or pattern stabilizes, or maybe until fly-by detected
      for (i = 1; i <= MAXGEN+2; i++)
	{
	  if (!pat_generate (&lab [i-1], &lab [i]))
	    {
	      dies_at (r, i);
	      free_target (&tgt);
	      return;
	    }

	  // Stabilized? Just check for P2 here, because that's cheap and anything P3 or beyond is too rare to bother.
	  if (i-2 >= 0 && pat_compare (&lab [i], &lab [i-2]))
	    {
	      p = 2;
	      if (pat_compare (&lab [i-1], &lab [i-2]))
		p = 1;

	      stabilizes (r, &tgt, i-2, p);
	      free_target (&tgt);
	      return;
	    }

	  // Fly-by detection 
	  if (i == flyGen && pat_match (&lab [flyGen], flyX, flyY, bullets [r->b].p))
	    {
	      pat_remove (&lab [flyGen], flyX, flyY, bullets [r->b].p);

	      // Synchronise reaction with the uncollided target
	      while (flyGen % tgt.nph)
		{
		  if (!pat_generate (&lab [flyGen], &lab [flyGen+1]))
		    {
		      dies_at (r, flyGen+1);
		      free_target (&tgt);
		      return;
		    }

		  flyGen++;
		}

	      if (pat_match (&lab [flyGen], tgt.X, tgt.Y, tgt.pat))
		{
		  pat_remove (&lab [flyGen], tgt.X, tgt.Y, tgt.pat);
		  if (W(&lab [flyGen]) <= 0)
		    {
		      if (!fly_by (r, &tgt, i))
			{
			  free_target (&tgt);
			  return;
			}

		      // Here: we have a fly-by condition, and fly_by () has decided to let us rerun the reaction.
		      // r is already updated to reflect this.
		      re_fly = true;
		      break;
		    }
		}

	      // We did find the bullet where we would expect it if this were a fly-by.
	      // But the remaining pattern after removal of that bullet did not match the original target.
	      // So let's forget about it! (i.e.: take one step back, and suppress fly-by detection)
	      i--;
	      flyGen = -1;
	    }
	}
    }
  while (re_fly);

  // Maybe the reaction emitted a space ship?
  // We not only have to handle the emitted ship, but we also might have missed our pattern stabilizing some time ago.
  i = search_ships (r, &tgt, MAXGEN+2);
  if (i < 0)
    {
      free_target (&tgt);
      return; // reaction was pruned.
    }
  for (; i < MAXGEN; i++)
    {
      pat_shrink_bbox (&lab [i]);
      if (!W(&lab [i]))
	{
	  dies_at (r, i);
	  free_target (&tgt);
	  return;
	}
    }
  p = 1 + i - MAXGEN;

  // OK. check lab [MAXGEN] for p <= MAXPERIOD ...
  for (; p <= MAXPERIOD; p++)
    {
      // Well. It COULD die even now ... so what.
      if (!pat_generate (&lab [MAXGEN + p-1], &lab [MAXGEN + p]))
	{
	  dies_at (r, MAXGEN + p);
	  free_target (&tgt);
	  return;
	}

      // Check if lab [] repeats with delta = p
      if (pat_compare (&lab [MAXGEN + p], &lab [MAXGEN]))
	{
	  // It really is a high period osci!
	  // Trace back to where it started!
	  for (i = MAXGEN; i > 0; i--)
	    if (!pat_compare (&lab [i-1], &lab [i-1+p]))
	      break;

	  stabilizes (r, &tgt, i, p);
	  free_target (&tgt);
	  return;
	}
    }

  // Me didn't find nuffin'!
  unstable (r, MAXGEN);
  free_target (&tgt);
}


void init_reactions (void)

{
  // if this fails we need to redefine struct reaction in reaction.h
  assert (LANES <= UINT8_MAX);

  tgts = calloc (MAXPERIOD, sizeof (target *));
          
  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();

// {int i; for (i = 0; ships [i].id > 0; i++) printf ("%d: Phase %d of '%s': (%d, %d)\n", i, ships [i].phase, ships [i-ships [i].phase].name, ships [i].offX, ships [i].offY);}
}
