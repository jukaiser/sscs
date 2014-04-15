/* main body of a program to find the recipe leading to a given reaction */

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


#define MAXPARTS 100


#define SELECT "SELECT reaction.rId, initial_tId, initial_phase, bId, lane, initial_state, rephase, delay, pId, cost, total_cost FROM transition LEFT JOIN reaction USING (rId) "
#define FIRST  "WHERE rId = %llu ORDER BY cost DESC LIMIT 1"
#define NEXT   "WHERE result_tId = %llu AND (result_state+%u-lane_adj)%%%u = %u AND total_cost = %u ORDER BY cost DESC LIMIT 1"


typedef struct
  {
    ROWID rId, tId;
    int	  phase;
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
static int   n_recipe = 0;
static object *ships = NULL;
static part   *parts = NULL;



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
  t->bId        = strtoull (row [3], NULL, 0);
  t->lane       = atoi (row [4]);
  t->state      = atoi (row [5]);
  t->rephase    = atoi (row [6]);
  t->delay      = atoi (row [7]);
  t->p          = find_part (strtoull (row [8], NULL, 0));
  t->cost       = atoi (row [9]);
  t->total_cost = atoi (row [10]);

  mysql_free_result (result);

  return t;
}


#if 0
ROWID tIds [P_MAX];
int   n_tIds = 0;
MYSQL *con;
bullet bullets [1];

pattern rephaser;
int rephaser_X;
pattern R1L0 [P_MAX];
int R1L0_X [P_MAX];
pattern constructor;
pattern trail;


static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%s\n", mysql_error (con));
  mysql_close (con);
  exit (2);
}


static void load_bullet (bullet *b, ROWID bId)

{
  char query [4096];
  MYSQL_RES *res;
  MYSQL_ROW row;

  b->id = bId;

  snprintf (query, 4095, "SELECT b.oId, rle, dx, dy, dt, base_x, base_y, reference, lane_dx, lane_dy, lanes_per_height, lanes_per_width, extra_lanes "
				"FROM bullets b LEFT JOIN objects USING (oId) WHERE bId = %llu", b->id);

  if (mysql_query (con, query))
    finish_with_error (con);

  res = mysql_store_result (con);
  if (!res)
    return;

  row = mysql_fetch_row (res);
  if (!row)
    return;
  b->oId = strtoull (row [0], NULL, 0);
  pat_from_string (row [1]);
  b->p = pat_compact (&lab [0], NULL);
  b->dx = atoi (row [2]);
  b->dy = atoi (row [3]);
  b->dt = atoi (row [4]);
  b->base_x = atoi (row [5]);
  b->base_y = atoi (row [6]);
  if (strcmp (row [7], "TOPLEFT") == 0)
    b->reference = topleft;
  else if (strcmp (row [7], "TOPRIGHT") == 0)
    b->reference = topright;
  else if (strcmp (row [7], "BOTTOMLEFT") == 0)
    b->reference = bottomleft;
  else if (strcmp (row [7], "BOTTOMRIGHT") == 0)
    b->reference = bottomright;
  b->lane_dx = atoi (row [8]);
  b->lane_dy = atoi (row [9]);
  b->lanes_per_height = atoi (row [10]);
  b->lanes_per_width = atoi (row [11]);
  b->extra_lanes = atoi (row [12]);
  mysql_free_result (res);
}


static ROWID parent_reaction_of (ROWID rId)
// search reaction which not only yields the initial target for reaction rId as a result, but also has the correct cost.

{
  char query [4096];
  MYSQL_RES *res;
  MYSQL_ROW row;
  ROWID ret = 0, i_tId, r_tId, tId;
  int   cost, new_lane;
  target old, new;
 
  snprintf (query, 4095, "SELECT initial_tId, lane, cost FROM reaction WHERE rId = %llu", rId);
  if (mysql_query (con, query))
    finish_with_error (con);

  res = mysql_store_result (con);
  if (!res)
    return ret;

  row = mysql_fetch_row (res);
  if (!row)
    return ret;

  tId = strtoull (row [0], NULL, 0);
  new_lane = atoi (row [1]);
  cost = atoi (row [2]);
  mysql_free_result (res);

  load_targets_ids (tId);

  load_tgt_bbox (&old, tId, 0, 0);

  if (n_tIds == 1)
    snprintf (query, 4095, "SELECT rId, initial_tId, result_tId, bId, lane, offX, offY, gen, cost FROM reaction WHERE result_tId = %llu AND cost < %d", tId, cost);
  else
    {
      char tmp [100];
      int n;

      strcpy (query, "SELECT rId, initial_tId, result_tId, bId, lane, offX, offY, gen, cost FROM reaction WHERE result_tId in (");
      for (n = 0; n < n_tIds; n++)
	{
	  snprintf (tmp, 99, "%s%llu", n?", ":"", tIds [n]);
	  strcat (query, tmp);
	}
      snprintf (tmp, 99, ") AND cost < %d", cost);
      strcat (query, tmp);
    }

  if (mysql_query (con, query))
    finish_with_error (con);

  res = mysql_store_result (con);
  if (!res)
    return ret;

  while (!ret && (row = mysql_fetch_row(res)))
    {
      ROWID toChk = strtoull (row [0], NULL, 0);
      i_tId = strtoull (row [1], NULL, 0);
      r_tId = strtoull (row [2], NULL, 0);
      ROWID bId = strtoull (row [3], NULL, 0);
      if (bId != bullets [0].id)
	load_bullet (&bullets [0], bId);
      int old_lane = strtoull (row [4], NULL, 0);
      int offX = strtoull (row [5], NULL, 0);
      int offY = strtoull (row [6], NULL, 0);
      int gen = strtoull (row [7], NULL, 0);
      int delta = cost - strtoull (row [8], NULL, 0);

      load_tgt_bbox (&new, i_tId, -offX, -offY);

      if (delta == cost_for (old_lane + tgt_adjust_lane (0, &old, &new), new_lane))
	{
	  int i, delay=0;
	  // The result_tId we just received corresponds to the generation in which the reaction stabilizes.
	  // The space ship parts have a period of FACTOR * DT
	  // Construction is assumed to be at multiple of this.
	  // Thus we have to synchronize and the check where we get the right phase or have to delay the
	  // next rake for a few ticks.
	  // If the target is a still life, we don't have to bother, tho.
	  if (n_tIds != 1)
	    {
	      // find current phase (r_tId) in tIds []
	      i = 0;
	      while (i < n_tIds && r_tId != tIds [i])
		i++;

	      // synchronise gen with period of space ship
	      i += (FACTOR*DT - gen);
	      while (i < 0)
		i += n_tIds;
	      i %= n_tIds;

	      // tIds [i] represends the phase our result will be in when we would fire the next rake without
	      // any delay.
	      // Thus, to calculate the delay we must cycle thru tIds [] until we find the original tId used
	      // by the reaction we are the parent of. (sounds complicated, eh?)
	      while (tId != tIds [(i+delay)%n_tIds])
		delay++;
	    }

	  ret = toChk;
	  recipe [n_recipe  ].rId = toChk;
	  recipe [n_recipe  ].tId = i_tId;
	  recipe [n_recipe  ].bId = bId;
	  recipe [n_recipe  ].offX = offX;
	  recipe [n_recipe  ].offY = offY;
	  recipe [n_recipe  ].lane = old_lane - tgt_adjust_lane (0, &old, &new);
	  recipe [n_recipe  ].delta = delta;
	  recipe [n_recipe  ].nph = n_tIds;
	  recipe [n_recipe++].delay = delay;
	}
    }

  mysql_free_result (res);

  return ret;
}
#endif

 
main (int argc, char **argv)

{
  char query [4096];
  int  i, n;
  trace *t;

  config_load (argv [1]);
  db_init ();

  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);

  // if this fails we need to redefine struct reaction in reaction.h
  assert (LANES <= UINT8_MAX);

  // ALLOC (target *, tgts, MAXPERIOD)

  // load all standard ships so we can search for them ;)
  ships = db_load_space_ships ();

  // load all parts we can use for our ship-to-build.
  bullets = db_load_bullets_for (SHIPNAME);
  parts = db_load_parts_for (SHIPNAME);

  assert (bullets [1].id == 0);

#if 0
  // Make room for our the ship parts
  MAXWIDTH = PARTWIDTH;
  MAXHEIGHT = PARTHEIGHT;
  MAXGEN = MAXPERIOD;
  MAX_RLE = 65536;
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);
  pat_allocate (&constructor, PARTWIDTH, PART_DY*MAXPARTS+PARTHEIGHT);

  pat_from_string (TRAIL_RLE);
  pat_compact (&lab [0], &trail);
  pat_from_string (REPHASE_RLE);
  pat_compact (&lab [0], &rephaser);
  rephaser_X = xPos = pat_first_X (&rephaser, 0);
//printf ("rephaser: %d\n", rephaser_X);
// pat_dump (&rephaser, true);
  pat_from_string (R1L0_RLE);
  pat_compact (&lab [0], &R1L0 [0]);
  for (i = 1; i < MAXPERIOD; i++)
    {
      pat_generate (&lab [i-1], &lab [i]);
      pat_compact (&lab [i], &R1L0 [i]);
    }
  for (i = 0; i < MAXPERIOD; i++)
    {
      R1L0_X [i] = pat_first_X (&R1L0 [i], 0);
      if (R1L0_X [i] > xPos)
	xPos = R1L0_X [i];
//printf ("R1L0 [%d]: %d {%d}\n", i, R1L0_X [i], xPos);
    }
// pat_dump (&R1L0 [0], true);
// exit (0);
#endif

  for (i = 2; i < argc; i++)
    {
      sprintf (query, SELECT FIRST, strtoull (argv [i], NULL, 0));

      t = follow (query);

      while (t->total_cost > 0)
	{
	  sprintf (query, SELECT NEXT, t->tId, LANES, LANES, t->state, t->total_cost-t->cost);
	  t = follow (query);
	}

      for (n = n_recipe-1; n >= 0; n--)
	printf ("#C [%llu]: REPHASE BY %u LANES, DELAY %u TICKS, FIRE %s\n", recipe [n].rId, recipe [n].rephase, recipe [n].delay, recipe [n].p->name);

#if 0
      int delay = 0;

      ROWID rId = strtoull (argv [i], NULL, 0);
      n_recipe = 0;
      recipe [n_recipe++].rId = rId;
      do
	rId = parent_reaction_of (rId);
      while (rId);

      if (n_recipe <= 1)
	{
	  printf ("No need to back track reaction %s!\n", argv [i]);
	  continue;
	}

      pat_init (&constructor);
      yPos = 0;
      pat_copy (&constructor, xPos-R1L0_X [0], yPos, &R1L0 [0]);
//printf ("R1L0 [0]: %d = %d-%d\n", xPos-R1L0_X [0], xPos, R1L0_X [0]);
      yPos += PART_DY;
      --n_recipe;
      printf ("#C Start with reaction %llu\n", recipe [n_recipe].rId);
      while (n_recipe > 0)
	{
	  assert (recipe [n_recipe].bId == 1);
	  delay = (delay + recipe [n_recipe].delay) % recipe [n_recipe].nph;
	  if (recipe [n_recipe].delta > 1) printf ("#C REPHASE x%d\n", recipe [n_recipe].delta-1);
	  if (delay) printf ("#C DELAY %dgen\n", delay);
	  // printf ("#C FIRE 1RL0\n");
	  for (j = 0; j < recipe [n_recipe].delta-1; j++)
	    {
	      pat_add (&constructor, xPos-rephaser_X, yPos, &rephaser);
// printf ("rephase: %d = %d-%d\n", xPos-rephaser_X, xPos, rephaser_X);
	      yPos += PART_DY;
	    }
	  // TO DO: carry delay over to next part!
	  pat_add (&constructor, xPos-R1L0_X [delay], yPos, &R1L0 [delay]);
// printf ("R1L0 [%d]: %d = %d-%d\n", j, xPos-R1L0_X [j], xPos, R1L0_X [j]);
	  yPos += PART_DY;
	  n_recipe--;
	  printf ("#C FIRE 1RL0 [resulting reaction:%llu]\n", recipe [n_recipe].rId);
	}

      for (j = 0; j < H(&constructor)/30; j++)
	{
	  int k;
	  pat_dump (&trail, false);
	  for (k = 0; k < 29; k++)
	    printf (".\n");
	}
      pat_dump (&constructor, false);
      printf ("\n");
      // printf ("x = %d, y = %d, rule = B3/S23\n%s\n", W(&constructor), H(&constructor), pat_rle (&constructor));
#endif
    }
}
