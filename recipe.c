/* main body of a program to find the recipe leading to a given reaction */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"


#define SHIP_MAGIC_X		60	// TODO: currently magic value >= -offX for ALL parts
#define SHIP_MAGIC_SZ		9	// TODO: currently magic value ~ max (H(part) / part->cost)
#define PARTWIDTH		1200	// TODO: currently magic value > max (W(part))
#define PARTHEIGHT		1100	// TODO: currently magic value > max (H(part))

#define MAXPARTS   100


#define SELECT "SELECT reaction.rId, initial_tId, %u+gen-result_phase, period, bId, lane, initial_state, rephase, initial_phase, pId, cost, total_cost " \
			"FROM transition LEFT JOIN reaction USING (rId) LEFT JOIN target ON (initial_tId = tId) "
// #define FIRST  "WHERE rId = %llu ORDER BY cost DESC LIMIT 1"
// #define NEXT   "WHERE result_tId = %llu AND (result_state+%u-lane_adj)%%%u = %u AND total_cost = %u ORDER BY cost DESC LIMIT 1"
#define FIRST  "WHERE rId = %llu ORDER BY cost ASC LIMIT 1"
#define NEXT   "WHERE result_tId = %llu AND (result_state+3*%u-lane_adj)%%%u = %u AND total_cost = %u ORDER BY cost ASC LIMIT 1"


typedef struct
  {
    ROWID rId, tId;
    int	  rPhase, iPhase;
    int   period;
    ROWID bId;
    int   lane;
    int	  state;
    int   rephase;
    int   delay;
    part  *p;
    int   cost;
    int   total_cost;
  } trace;


static trace recipe [MAXPARTS];
static int n_recipe = 0;
static object *ships = NULL;
static part *parts = NULL;
static pattern constructor;
static bool with_demo = false;


uint8_t mod (int value, int divisor)
// C operator % does not what we want, if LHS < 0

{
  // Sanity. Avoid Division by zero and negative divisors:
  // Plus: x mod 1 == 0 ...
  assert (divisor > 0);
  if (divisor <= 1)
    return 0;

  // C operator % does not what we want, if LHS < 0
  while (value < 0)
    value += divisor;

  return value % divisor;
}


part *find_part (ROWID pId)

{
  int p;
  for (p = 0; parts [p].pId; p++)
    if (parts [p].pId == pId)
      return &parts [p];

  return NULL;
}


trace *follow (const char *q)

{
  trace *t;

  if (mysql_query (con, q))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "No result for '%s'\n", q);
      exit (2);
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  if (!row)
    {
      fprintf (stderr, "No result for '%s'\n", q);
      exit (2);
    }

  t = &recipe [n_recipe++];
  t->rId        = strtoull (row [0], NULL, 0);
  t->tId        = strtoull (row [1], NULL, 0);
  t->rPhase     = atoi (row [2]);
  t->period     = atoi (row [3]);
  t->bId        = strtoull (row [4], NULL, 0);
  t->lane       = atoi (row [5]);
  t->state      = atoi (row [6]);
  t->rephase    = atoi (row [7]);
  t->iPhase     = atoi (row [8]);
  t->delay      = 0;
  t->p          = find_part (strtoull (row [9], NULL, 0));
  t->cost       = atoi (row [10]);
  t->total_cost = atoi (row [11]);

  mysql_free_result (result);

  return t;
}


static void Usage (const char *argv0)

{
  fprintf (stderr, "Usage: %s <configfile> <rId> [<rId> ...]\n\n\tDump the recipe for the given reaction[s].\n\n", argv0);
  fprintf (stderr, "Usage: %s --demo <configfile> <rId> [<rId> ...]\n\n\tDump the recipe for the given reaction[s] including a complete demo for the construction.\n\n", argv0);
  exit (1);
}


main (int argc, char **argv)

{
  char query [4096];
  int  i, n, pos, firstArg;
  unsigned long stampY = 0UL;
  trace *first;
  part *track = NULL, *rephaser = NULL;
  target tgt;
  unsigned period;

  
  if (argc == 3 && argv [1][0] != '-')
    firstArg = 1;
  else if (argc == 4 && strcmp (argv [1], "--demo") == 0)
    {
      with_demo = true;
      firstArg = 2;
    }
  else
    Usage (argv [0]);

  config_load (argv [firstArg++]);
  db_init ();

  MAXWIDTH = PARTWIDTH;
  MAXHEIGHT = PARTHEIGHT;
  MAXGEN = MAXPERIOD*2;
  MAX_RLE = 65536;
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);
  pat_allocate (&constructor, PARTWIDTH, 2*(MAXPARTS+1)*PARTHEIGHT);

  // if this fails we need to redefine struct reaction in reaction.h
  assert (LANES <= UINT8_MAX);

  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();

  // Load THE bullet used by all our parts. Mixing rakes with differnt bullets is NYI.
  bullets = db_load_bullets_for (SHIPNAME);
  assert (bullets [1].id == 0);
  period = MAXPERIOD + mod (-MAXPERIOD,bullets[0].dt);

  // load all parts we can use for our ship-to-build.
  parts = db_load_parts_for (SHIPNAME, true, period, SHIP_MAGIC_X, abs (DY));

  // There must be a part of type pt_track and another one of type pt_rephaser for this ship.
  // TO DO: check for multiple versions ... Maybe different tracks would not make to much sense. Multiple rephasers DO.
  for (n = 0; parts [n].pId; n++)
    if (parts [n].type == pt_track)
      track  = &parts [n];
    else if (parts [n].type == pt_rephaser)
      rephaser  = &parts [n];

  assert (SHIPMODE);
  assert (DX == track->dx);
  assert (DY == -track->dy);
  assert (track);
  assert (track->dx == 0);
  assert (track->dy > 0);	// NYI: only northbound ships please

  for (i = firstArg; i < argc; i++)
    {
      int delay = 0, dummy;

      sprintf (query, SELECT FIRST, DT, strtoull (argv [i], NULL, 0));

      n_recipe = 0;
      first = follow (query);

      while (first->total_cost > 0)
	{
	  sprintf (query, SELECT NEXT, DT, first->tId, LANES, LANES, first->state, first->total_cost-first->cost);
	  first = follow (query);
	}

      // Write recipe in the header.
      printf ("#P 0 %lu\n", stampY);
      db_target_reload (&tgt, first->tId, first->iPhase);
      tgt_collide (&tgt, &bullets [0], first->lane, &dummy, &dummy, &dummy, &dummy);
      printf ("#C Start-RLE: %s\n", pat_rle (&lab [0]));
      for (n = n_recipe-1; n >= 0; n--)
	{
	  if (n == n_recipe-1)
	    delay = mod (recipe [n].iPhase, recipe [n].period);
	  else
	    delay = mod (recipe [n+1].rPhase - delay - recipe [n].iPhase, recipe [n].period);
	  recipe [n].delay = delay;

	  printf ("#C [%llu]: ", recipe [n].rId);
	  if (recipe [n].rephase)
	    printf ("REPHASE BY %u LANES, ", recipe [n].rephase);
	  if (delay)
	    printf ("DELAY %d TICKS, ", delay);
	  printf ("FIRE %s\n", recipe [n].p->name);
	}

      // Construct demo.
      if (with_demo)
	{
	  pat_init (&constructor);
	  pos = 0;
	  for (n = 0; n < (recipe [0].total_cost+5)*SHIP_MAGIC_SZ; n++)
	    {
	      pat_add (&constructor, track->pats [0].X, pos+track->pats [0].Y, track->pats [0].pat);
	      pos += track->dy;
	    }

	  for (n = n_recipe-1; n >= 0; n--)
	    {
	      int phase = recipe [n].rephase;
	      while (phase != 0)
		{
		  pat_add (&constructor, rephaser->pats [0].X, pos+rephaser->pats [0].Y, rephaser->pats [0].pat);
		  pos += rephaser->dy;
		  phase = (LANES + phase - rephaser->lane_adjust) % LANES;
		}

	      pat_add (&constructor, track->pats [0].X, pos+track->pats [0].Y, track->pats [0].pat);
	      pos += track->dy;

	      part *p = recipe [n].p;
	      target *pt = &p->pats [period - recipe [n].delay];
	      pat_add (&constructor, pt->X, pos+pt->Y, pt->pat);

	      if (n == n_recipe-1)
		{
		  db_target_reload (&tgt, first->tId, 0);
		  int tgtX = p->fireX     + (period*bullets [0].dx/bullets [0].dt) - bullets [0].base_x - (recipe [n].lane*bullets [0].lane_dx) + tgt.X-tgt.left;
		  int tgtY = pos+p->fireY + (period*bullets [0].dy/bullets [0].dt) - bullets [0].base_y - (recipe [n].lane*bullets [0].lane_dy) + tgt.Y-tgt.bottom;
		  for (; tgtY > 0; tgtY += DY)
		    pat_add (&constructor, tgtX, tgtY, tgt.pat);
		}

	      pos += p->dy;
    /*
	      pat_add (&constructor, track->pats [0].X, pos+track->pats [0].Y, track->pats [0].pat);
	      pos += track->dy;
	      pat_add (&constructor, track->pats [0].X, pos+track->pats [0].Y, track->pats [0].pat);
	      pos += track->dy;
    */
	    }

	  // ... and show it.
	  pat_dump (&constructor, false);
	  stampY += 1000UL * (2UL + (unsigned long)pos / 1000UL);
	}
      else
	{
	  db_target_reload (&tgt, recipe [0].tId, recipe [0].iPhase);
	  tgt_collide (&tgt, &bullets [0], recipe [0].lane, &dummy, &dummy, &dummy, &dummy);
	  pat_dump (&lab [0], false);
	  stampY += 200;
	}
    }
}
