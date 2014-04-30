/* define and handle reactions */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"
#include "queue.h"

#include "profile.h"


static target **tgts;
static object *ships = NULL;
static part   *parts = NULL;
static found  *emitted;		// TO DO: ./. pattern.c::findings []
static int    n_emitted = 0;


typedef struct
  {
    uint8_t cost;
    uint8_t rephase_by;
    int     fire;
    uint8_t d_state;
  } transition;


transition *t_table = NULL;


uint8_t mod (int value, int divisor)
// C operator % does not do what we want, if LHS < 0

{
  // Sanity. Avoid Division by zero and negative divisors:
  // Plus: x mod 1 == 0 ...
  assert (divisor > 0);
  if (divisor <= 1)
    return 0;

  // We do not like negative values here!
  while (value < 0)
    value += divisor;

  return value % divisor;
}


transition *transition_for (int state, int lane)
// Our constructing engine is currently in a certaion state and is asked to fire on a certain lane.
// Calculate the relativ lane number (mod LANES) from this information an return the cheapest known
// transition for this.

{
  return &t_table [mod (lane - state, LANES)];
}


void reaction_targets_keep (int nph)

{
  int i, first;
  char *f_rle, *t_rle;
  ROWID id;
  
  // Find "first" phase - arbitrarily defined as the one choose by strcmp (rle1, rle2) as "minimal".
  first = 0;
  f_rle = strdup (pat_rle (tgts [0]->pat));
  for (i = 1; i < nph; i++)
    {
      t_rle = pat_rle (tgts [i]->pat);
      if (strcmp  (f_rle, t_rle) > 0)
	{
	  first = i;
	  free (f_rle);
	  f_rle = strdup (t_rle);
	}
    }

  // sync all we know about tgts with the DB.
assert (first < nph);
  id = db_target_store (pat_GSF_hash (tgts [first]->pat), f_rle, nph, W(tgts [first]), H(tgts [first]));
  free (f_rle);

  // Update id and phase of ALL targets.
  // tgt [first].phase ::= 0 because that is the phase we just saved to the DB!
  for (i = 0; i < nph; i++)
    {
      tgts [i]->id = id;
      tgts [i]->phase = mod (i - first, nph);
// if (nph > 2) printf ("id = %llu, first=%u, i=%d, phase=%d, nph=%d\n", tgts [i]->id, first, i, tgts [i]->phase, nph);
    }
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


void build_reactions (int nph, int b, bool preload, uint8_t old_cost, uint8_t state)

{
  uint8_t phase, j, cost;

  for (phase = 0; phase < nph; phase++)
    {
assert (tgts [phase]->id);
      int n = tgt_count_lanes (tgts [phase], b);

      if (LANES > 0 && LANES < n)
	n = LANES;
      for (j = 0; j < n; j++)
	{
	  if (!preload)
	    {
	      // queue reaction for later analysis ...
	      transition *t  = transition_for (state, j);
	      queue_insert (old_cost + t->cost, tgts [phase]->id, phase, state, b, j);
	    }
	  else
	    {
	      // we're preloading!
	      // Basically this means, that we have NO current state ... and therefore no sensible notion of a "cost".
	      int p;
	      for (p = 0; parts [p].pId; p++)
		if (parts [p].type == pt_rake)
		  queue_insert (0, tgts [phase]->id, phase, mod (j-parts [p].lane_fired, LANES), b, j);
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
      tgt->id = 0ULL;
      tgt->phase = -1;
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


static void dies_at (reaction *r, int i, result *res)

{
  res->type = rt_dies;
}


static void emit (ROWID rId, unsigned seq, int gen, int offX, int offY, ROWID oId)

{
  db_store_emit (rId, seq, oId, offX, offY, gen);
}


static bool fly_by (reaction *r, target *tgt, int i, result *res)

{
  res->gen = i;
  res->type = rt_flyby;


  if (SHIPMODE == 1 && r->lane + LANES < tgt_count_lanes (tgt, r->b))
    {
      // Keep old reaction so we can check for "we have ssen this before".
      db_reaction_keep (r, res);

      // refly with a new lane.
      r->lane += LANES;
      assert (r->lane <= UINT8_MAX);
      return true;
    }

  // not a reflyable fly-by ...
  return false;
}


static void prune (reaction *r, result *res)

{
  res->type = rt_pruned;
}


static void stabilizes (reaction *r, target *old, int i, int p, result *res)

{
  // The first nph generations of our lab [] starting with i contain the different phases of the resulting pattern
  // extract them as targets to tgts [] ...
  build_targets (i, p);

  // Tho the pattern did stabilize it might still be too big for us to follow it further.
  // TO DO: CHECK for other prune-conditions here!
  if (W(tgts [0]) > PRUNE_SX || H(tgts [0]) > PRUNE_SY)
    {
      prune (r, res);
      free_targets ();
      return;
    }

  // check with our db, retrieve the ROW-ids if the targets existed or store them if they are new.
  // NOTE: since checking + storing only if not there is more expensive then just storing (and failing if duplicate) we do not bother to check.
  // _prof_leave ("handle()");
  reaction_targets_keep (p);
  // _prof_enter ();

  // remember our success
  target *new = tgts [0];
  res->result_tId = new->id;
  res->result_phase = new->phase;	// U.Kaiser, 30.04.2014 - DARN!
  res->offX = new->left-old->left;
  res->offY = new->top-old->top;
  res->gen = i;
  res->lane_adj = tgt_adjust_lane (r->b, old, new);
  res->type = rt_stable;

  // build all possible reactions for these targets and queue them for later analysis.
  int state = r->state;
  state += transition_for (r->state, r->lane)->d_state;
  state -= res->lane_adj;
  state = mod (state, LANES);
  build_reactions (p, r->b, false, r->cost, state);
  free_targets ();
}


static void unstable (reaction *r, int i, result *res)

{
  res->gen = i;
  res->type = rt_unfinished;
}


static int search_ships (reaction *r, target *tgt, int gen, result *res)

{
  int i;

  // Search for escaping space ships
  emitted = obj_search (gen, ships, &n_emitted);
  res->emits = (n_emitted >= 1);
  if (!emitted)
    {
      // obj_search () found something we do NOT like to find. Discard this reaction!
      prune (r, res);
      return -1;
    }
  if (n_emitted < 1)
    return gen;

  // We know that at the current generation all objects in emitted have been around.
  // Find the point in time where the last ship appeared and remove them all.
  gen = obj_back_trace ();

  return gen;
}


bool run (reaction *r, target *tgt, result *res)

{
  int flyX, flyY, flyGen, i, p;

  // Build new collision defined by our reaction -> lab [0], reinitializing lab, tgt and n_emitted
  lab_init ();
  tgt_collide (tgt, &bullets [r->b], r->lane, &res->delay, &flyX, &flyY, &flyGen);
  if (tgt->nph == 1)
    res->delay = 0;
  else
    res->delay = mod (res->delay, tgt->nph);
  n_emitted = 0;
  res->emits = false;

  // Generate until MAXGEN or pattern stabilizes, or maybe until fly-by detected
  for (i = 1; i <= MAXGEN+2; i++)
    {
// pat_dump (&lab [i-1], true);
      if (!pat_generate (&lab [i-1], &lab [i]))
	{
	  dies_at (r, i, res);
	  return false;
	}

      // Stabilized? Just check for P2 here, because that's cheap and anything P3 or beyond is too rare to bother.
      if (i-2 >= 0 && pat_compare (&lab [i], &lab [i-2]))
	{
	  p = 2;
	  if (pat_compare (&lab [i-1], &lab [i-2]))
	    p = 1;

	  stabilizes (r, tgt, i-2, p, res);
	  return false;
	}

      // Fly-by detection 
      if (i == flyGen && pat_match (&lab [flyGen], flyX, flyY, bullets [r->b].p))
	{
	  pat_remove (&lab [flyGen], flyX, flyY, bullets [r->b].p);

	  // Synchronise reaction with the uncollided target
	  while (flyGen % tgt->nph)
	    {
	      if (!pat_generate (&lab [flyGen], &lab [flyGen+1]))
		{
		  dies_at (r, flyGen+1, res);
		  return false;
		}

	      flyGen++;
	    }

	  if (pat_match (&lab [flyGen], tgt->X, tgt->Y, tgt->pat))
	    {
	      pat_remove (&lab [flyGen], tgt->X, tgt->Y, tgt->pat);
	      if (W(&lab [flyGen]) <= 0)
		{
		  // Here: we have a fly-by condition
		  // Let fly_by () decide if we should rerun the reaction on a new lane - fly_by () will adjust r->lane if
		  // required.
		  return fly_by (r, tgt, i, res);
		}
	    }

	  // We did find the bullet where we would expect it if this were a fly-by.
	  // But the remaining pattern after removal of that bullet did not match the original target.
	  // So let's forget about it! (i.e.: take one step back, and suppress fly-by detection)
	  i--;
	  flyGen = -1;
	}
    }

  // Maybe the reaction emitted a space ship?
  // We not only have to handle the emitted ship, but we also might have missed our pattern stabilizing some time ago.
  i = search_ships (r, tgt, MAXGEN+2, res);
  if (i < 0)
    return false; // reaction was pruned.
  for (; i < MAXGEN; i++)
    {
      pat_shrink_bbox (&lab [i]);
      if (!W(&lab [i]))
	{
	  dies_at (r, i, res);
	  return false;
	}
    }
  p = 1 + i - MAXGEN;

  // OK. check lab [MAXGEN] for p <= MAXPERIOD ...
  for (; p <= MAXPERIOD; p++)
    {
      // Well. It COULD die even now ... so what.
      if (!pat_generate (&lab [MAXGEN + p-1], &lab [MAXGEN + p]))
	{
	  dies_at (r, MAXGEN + p, res);
	  return false;
	}

      // Check if lab [] repeats with delta = p
      if (pat_compare (&lab [MAXGEN + p], &lab [MAXGEN]))
	{
	  // It really is a high period osci!
	  // Trace back to where it started!
	  for (i = MAXGEN; i > 0; i--)
	    if (!pat_compare (&lab [i-1], &lab [i-1+p]))
	      break;

	  stabilizes (r, tgt, i, p, res);
	  return false;
	}
    }

  unstable (r, MAXGEN, res);
  return false;
}


void handle (reaction *r)

{
  result res = {0ULL, 0, 0, 0, 0, 0, 0, false, rt_undef};

  target tgt;
  bool   re_fly = false;
  int    i;
  unsigned cost;
  ROWID  rId;
  transition *t;

  assert (r);

  // Maybe the collision has been queued more then once.
  // And maybe *we* are not handling the cheapest of those.
  // Since we are queueing reactions in order of least cost we are able to check this.
  // NOTE: since "re-flying" is handled immediately, we only have to check this for the very
  //	   first pass.
  rId = db_is_reaction_finished (r->tId, r->phase, r->b, r->lane, &cost);
  if (rId)
    {
      t = transition_for (r->state, r->lane);
      assert (cost <= r->cost);
      if (cost == r->cost)
	db_store_transition (rId, r->state, mod (r->state+t->d_state, 31), t->rephase_by, parts [t->fire].pId, r->cost, t->cost);
      return;
    }

  // Fetch our target from the datebase.
  db_target_reload (&tgt, r->tId, r->phase);

  // if we detect a fly-by condition we might want to immediately rerun the reaction on a higher lane ...
  do
    {
      // _prof_enter ();
      re_fly = run (r, &tgt, &res);
      // _prof_leave ("handle()");
    }
  while (re_fly);

  // Store our reaction, now that we know everything about it!
  rId = db_reaction_keep (r, &res);
  t = transition_for (r->state, r->lane);
  db_store_transition (rId, r->state, mod (r->state+t->d_state, 31), t->rephase_by, parts [t->fire].pId, r->cost, t->cost);

  // If we emitted any ships then store them.
  // Note that we want *relative* postions for our DB!
  for (i = 0; i < n_emitted; i++)
    emit (rId, i, emitted [i].gen, emitted [i].offX-tgt.X, emitted [i].offY-tgt.Y, emitted [i].obj->id);

  // We don't need the current target any longer.
  free_target (&tgt);
}


static void calc_transitions (int lane, int base_cost)
// recursively find the cheapest way to fire a bullet on a given lane, if starting from a given lane.

{
  int p;

  // First handle all rakes
  for (p = 0; parts [p].pId; p++)
    if (parts [p].type == pt_rake)
      if (t_table [(lane + parts [p].lane_fired) % LANES].fire < 0 || t_table [(lane + parts [p].lane_fired) % LANES].cost > base_cost + parts [p].cost)
	{
// SOME assert()ions please?
	  t_table [(lane + parts [p].lane_fired) % LANES].cost	     = base_cost + parts [p].cost;
	  t_table [(lane + parts [p].lane_fired) % LANES].rephase_by = lane;
	  t_table [(lane + parts [p].lane_fired) % LANES].fire	     = p;
	  t_table [(lane + parts [p].lane_fired) % LANES].d_state    = (lane + parts [p].lane_adjust) % LANES;
	}
      else if (t_table [(lane + parts [p].lane_fired) % LANES].cost == base_cost + parts [p].cost)
	printf ("Warning: more then one way to reach lane offset %d with cost=%d.\nRandomly taking '%s' instead of '%s'!\n",
		(lane + parts [p].lane_fired) % LANES, t_table [(lane + parts [p].lane_fired) % LANES].cost, parts [t_table [(lane + parts [p].lane_fired) % LANES].fire].name, parts [p].name);

  // Then recurse for all rephasers.
  // TO CHECK: I never tried it with more then one kind of rephaser, yet. YMMV
  for (p = 0; parts [p].pId; p++)
    if (parts [p].type == pt_rephaser)
      if ((lane + parts [p].lane_adjust) % LANES != 0)
	calc_transitions ((lane + parts [p].lane_adjust) % LANES, base_cost + parts [p].cost);
}


void init_reactions (void)

{
  int l;

  // if this fails we need to redefine struct reaction in reaction.h
  assert (LANES <= UINT8_MAX);

  ALLOC (target *, tgts, MAXPERIOD)

  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();

  // load all parts we can use for our ship-to-build.
  bullets = db_load_bullets_for (SHIPNAME);
  parts = db_load_parts_for (SHIPNAME, false, 0, 0, 0);

  assert (bullets [1].id == 0);
  /*
     NOTE: implementing multiple bullets:
	- the cost-calculations (transition_for () and friends) and tgt_adjust_lane () would have to be done for EVERY bullet (at least for every variant
	  of bullet with a differnt value for (lane_dx, lane_dy))
	- As a consequence state would have to be tracked per bullet, or made at least two dimensional (= offset between track and construction site).
	- Generally: everything with a notion of "lane" or "state" has to be reviewed.
	- Enumeration of reactions (i.e. build_reactions ()) has to be done for all bullets.
  */

  // Calculate the costs for firing on a given lane based on the current "state" of the tracks.
  ALLOC(transition,t_table,LANES)

  for (l = 0; l < LANES; l++)
    t_table [l].fire = -1;

  calc_transitions (0, 0);

  // for (l = 0; l < LANES; l++)
    // printf ("dLane = %d: cost=%d, firing=%s, state=%d\n", l, t_table [l].cost, parts [t_table [l].fire].name, t_table [l].d_state);
}
