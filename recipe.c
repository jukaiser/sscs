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


#define SELECT "SELECT reaction.rId, initial_tId, 240-gen+result_phase, period, bId, lane, initial_state, rephase, delay, pId, cost, total_cost " \
			"FROM transition LEFT JOIN reaction USING (rId) LEFT JOIN target ON (initial_tId = tId) "
// #define FIRST  "WHERE rId = %llu ORDER BY cost DESC LIMIT 1"
// #define NEXT   "WHERE result_tId = %llu AND (result_state+%u-lane_adj)%%%u = %u AND total_cost = %u ORDER BY cost DESC LIMIT 1"
#define FIRST  "WHERE rId = %llu ORDER BY cost ASC LIMIT 1"
#define NEXT   "WHERE result_tId = %llu AND (result_state+%u-lane_adj)%%%u = %u AND total_cost = %u ORDER BY cost ASC LIMIT 1"


typedef struct
  {
    ROWID rId, tId;
    int	  phase;
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
  t->phase      = atoi (row [2]);
  t->period     = atoi (row [3]);
  t->bId        = strtoull (row [4], NULL, 0);
  t->lane       = atoi (row [5]);
  t->state      = atoi (row [6]);
  t->rephase    = atoi (row [7]);
  t->delay      = atoi (row [8]);
  t->p          = find_part (strtoull (row [9], NULL, 0));
  t->cost       = atoi (row [10]);
  t->total_cost = atoi (row [11]);

  mysql_free_result (result);

  return t;
}


main (int argc, char **argv)

{
  char query [4096];
  int  i, n, pos;
  unsigned long stampY = 0UL;
  trace *t;
  part *track = NULL, *rephaser = NULL;
  target tgt;
  unsigned period;

  config_load (argv [1]);
  db_init ();

  MAXWIDTH = PARTWIDTH;
  MAXHEIGHT = PARTHEIGHT;
  MAXGEN = MAXPERIOD*2;
  MAX_RLE = 65536;
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);
  pat_allocate (&constructor, PARTWIDTH, 2*(MAXPARTS+1)*PARTHEIGHT);

  // if this fails we need to redefine struct reaction in reaction.h
  assert (LANES <= UINT8_MAX);

  // ALLOC (target *, tgts, MAXPERIOD)

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

  for (i = 2; i < argc; i++)
    {
      int delay = 0;

      sprintf (query, SELECT FIRST, strtoull (argv [i], NULL, 0));

      n_recipe = 0;
      t = follow (query);

      while (t->total_cost > 0)
	{
	  sprintf (query, SELECT NEXT, t->tId, LANES, LANES, t->state, t->total_cost-t->cost);
	  t = follow (query);
	}
      db_target_reload (&tgt, t->tId, 0);

      // Write recipe in the header.
      printf ("#P 0 %lu\n", stampY);
      for (n = n_recipe-1; n >= 0; n--)
	{
	  delay += recipe [n].delay;
	  if (n < n_recipe-1)
	    delay -= recipe [n+1].phase;
	  delay = mod (delay, recipe [n].period);
	  recipe [n].delay = delay;

	  printf ("#C [%llu]: ", recipe [n].rId);
	  if (recipe [n].rephase)
	    printf ("REPHASE BY %u LANES, ", recipe [n].rephase);
	  if (delay)
	    printf ("DELAY %d TICKS, ", delay);
	  printf ("FIRE %s\n", recipe [n].p->name);
	}

      // Construct demo.
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

	  part *p = recipe [n].p;
	  target *pt = &p->pats [period - recipe [n].delay];
	  pat_add (&constructor, pt->X, pos+pt->Y, pt->pat);

	  if (n == n_recipe-1)
{
fprintf (stderr, "(%d,%d)\n", pt->X, pt->Y);
fprintf (stderr, "[%d,%d]\n", p->fireX, p->fireY);
fprintf (stderr, "{%d,%d} - {%d,%d}\n", tgt.X, tgt.Y, tgt.left, tgt.bottom);
fprintf (stderr, "X: %d+%d - (%d*%d) - %d + %d-%d -> %d\n", pt->X, p->fireX, recipe [n].lane, bullets [0].lane_dx, bullets [0].base_x, tgt.X, tgt.left, pt->X+p->fireX-(recipe [n].lane*bullets [0].lane_dx)-bullets [0].base_x+tgt.X-tgt.left);
fprintf (stderr, "MAGIC: %d, %d\n", (period*bullets [0].dx/bullets [0].dt), (period*bullets [0].dy/bullets [0].dt));
	    pat_add (&constructor,
		     pt->X+p->fireX+(period*bullets [0].dx/bullets [0].dt)-(recipe [n].lane*bullets [0].lane_dx)-bullets [0].base_x+tgt.X-tgt.left,
		     pos+pt->Y+p->fireY+(period*bullets [0].dy/bullets [0].dt)-(recipe [n].lane*bullets [0].lane_dy)-bullets [0].base_y+tgt.Y-tgt.bottom,
		     tgt.pat);
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
}
