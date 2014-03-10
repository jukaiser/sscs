/* main body of a program to find the recipe leading to a given reaction */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"


#define	PARTWIDTH	1000
#define PARTHEIGHT	300
#define PART_DY		270
#define	MAXPARTS	100
#define P_MAX		30

#define R1L0_RLE	"x = 736, y = 272, rule = B3/S23\n"						\
			"40b2o114b2o108b2o88b2o108b2o114b2o$40b2o114b2o108b2o88b2o108b2o114b2o\n"	\
			"26$585bo$584bobo$580b5obo$579bob2ob3o$156b2o108b2o88b2o108b2o114b2obob\n"	\
			"2o$156b2o108b2o88b2o108b2o113bob3o$581b2o$578bo2b3obo2bo$43bo535b3o2bo\n"	\
			"$42bobo537b2obo$38bo2b2ob2o539bo$38bo3bobo528b3o7bobo$38bo5bob2o536bo$\n"	\
			"43b2ob2o$45bo4$36b2o548b2o$36b2o548b2o7$40b2o540b2o$40b2o540b2o8$156b\n"	\
			"2o108b2o88b2o108b2o145bobo$156b2o108b2o88b2o108b2o146b2o$614bo2$5bo42b\n"	\
			"2o524b2o$3b2o43b2o524b2o$4b2o16$40b2o540b2o$40b2o540b2o$544bo$543bo$\n"	\
			"543b3o3$84bo$2o80bobo$obo80b2o71b2o108b2o88b2o108b2o$o155b2o108b2o88b\n"	\
			"2o108b2o3$48b2o524b2o$48b2o524b2o17$40b2o540b2o$40b2o106b3o431b2o$149b\n"	\
			"o$149b3o3$60b2o$60bobo$60bo413bo$266b2o88b2o115b3o$266b2o88b2o114b2o2b\n"	\
			"o$474b3o$158b2o304b2o$48b2o98b2o8b2o304b2o108b2o$48b2o98b2o424b2o3$\n"		\
			"466bo$464b2o$160bo304b2o$159bobo$158bo3bo$158bo3bo$158bo303b2o$159b3o\n"	\
			"300b2o7$40b2o114b2o308b2o114b2o$40b2o114b2o308b2o114b2o3$120b2o$120bob\n"	\
			"o$120bo$673bobo$355bo318b2o$354bo319bo$266bobo82bob2o2bo$263b2obo2bo\n"	\
			"80bobo2b4o$263b2obo2bo79bo2bo$48b2o98b2o112bob2o87b3o2b2o114b2o98b2o$\n"	\
			"48b2o98b2o112b4o5bo77b2obo4b2o115b2o98b2o$272b2o75b2obo4bo$265b2o6bo\n"	\
			"77b2obob2o$262bo3b2obob3o$263b5o70bobo$266bo71bob2o$339b2o6bo16b2o$\n"		\
			"346bo15bo3b2o2bo$346bo14bo2bob2obobo$262b2o97bo4bo4bo$262b2o93b2o9b3o$\n"	\
			"356b6o4b3o$356b2obo5b2o50bo$361b2o53bo$360b3o53b3o2$378bo$40b2o114b2o\n"	\
			"53bo54b2o88b2o21b2o85b2o16bo97b2o$40b2o114b2o51bobo54b2o88b2o20b2o86b\n"	\
			"2o15bo98b2o$180b2o28b2o271b3o$180bobo$180bo59bo$144bo93b2o$142bobo94b\n"	\
			"2o$143b2o6$48b2o98b2o124b2o72b2o124b2o98b2o$48b2o98b2o124b2o72b2o124b\n"	\
			"2o98b2o6$522bo$523b2o$522b2o3$95bobo$95b2o$96bo4$40b2o114b2o108b2o88b\n"	\
			"2o108b2o114b2o$40b2o114b2o108b2o88b2o108b2o114b2o12$48b2o224b2o72b2o\n"	\
			"224b2o$48b2o224b2o72b2o224b2o17$40b2o114b2o108b2o88b2o108b2o114b2o$40b\n"	\
			"2o114b2o108b2o88b2o108b2o114b2o4$733bobo$734b2o$734bo6$48b2o524b2o$48b\n"	\
			"2o524b2o17$40b2o114b2o108b2o88b2o108b2o114b2o$40b2o114b2o108b2o88b2o\n"	\
			"108b2o114b2o!\n"

#define REPHASE_RLE	"x = 619, y = 272, rule = B3/S23\n"						\
			"35b2o114b2o108b2o88b2o108b2o114b2o$35b2o114b2o108b2o88b2o108b2o114b2o\n"	\
			"27$33bo$32b3ob2o$32bo5bo$31b4o4bo111b2o108b2o88b2o108b2o$31bo7bo111b2o\n"	\
			"108b2o88b2o108b2o$31bo3b2o2b2o$31bo5bobo$32b2obo2bo536bo$574bobo$32b2o\n"	\
			"b2o536b2ob2o2bo$34bo8b3o528bobo3bo$571b2obo5bo$571b2ob2o$573bo4$31b2o\n"	\
			"548b2o$31b2o548b2o7$35b2o540b2o$35b2o540b2o7$5bo$3b2o146b2o108b2o88b2o\n"	\
			"108b2o$4b2o145b2o108b2o88b2o108b2o3$43b2o524b2o42bo$43b2o524b2o43b2o$\n"	\
			"613b2o16$35b2o540b2o$35b2o540b2o$74bo$72bobo$73b2o3$534bo$534bobo80b2o\n"	\
			"$151b2o108b2o88b2o108b2o71b2o80bobo$151b2o108b2o88b2o108b2o155bo3$2o\n"	\
			"41b2o524b2o$obo40b2o524b2o$o16$35b2o540b2o$35b2o431b3o106b2o$469bo$\n"		\
			"467b3o3$557b2o$556bobo$558bo$144bo116b2o88b2o$143b3o115b2o88b2o$60b2o\n"	\
			"80b2obo$60bobo90b2o304b2o$43b2o15bo92b2o304b2o8b2o98b2o$43b2o424b2o98b\n"	\
			"2o3$151bobo$152b2o$152bo305bo$457bobo$456bo3bo$456bo3bo$155b2o303bo$\n"	\
			"155b2o300b3o7$35b2o114b2o308b2o114b2o$35b2o114b2o308b2o114b2o3$497b2o$\n"	\
			"496bobo$498bo2$263bo$120b2o142bo$120bobo138bo2b2obo82bobo$120bo139b4o\n"	\
			"2bobo80bo2bob2o$266bo2bo79bo2bob2o$43b2o98b2o114b2o2b3o87b2obo112b2o\n"	\
			"98b2o$43b2o98b2o115b2o4bob2o77bo5b4o112b2o98b2o$261bo4bob2o75b2o$261b\n"	\
			"2obob2o77bo6b2o$345b3obob2o3bo$278bobo70b5o$277b2obo71bo$253b2o16bo6b\n"	\
			"2o$248bo2b2o3bo15bo$247bobob2obo2bo14bo$247bo4bo4bo97b2o$248b3o9b2o93b\n"	\
			"2o$250b3o4b6o$201bo50b2o5bob2o$199bobo54b2o$200b2o54b3o2$240bo$35b2o\n"	\
			"97bo16b2o85b2o21b2o88b2o54bo53b2o114b2o$35b2o95bobo16b2o86b2o20b2o88b\n"	\
			"2o54bobo51b2o114b2o$133b2o272b2o28b2o$436bobo$378bo59bo$379b2o93bo$\n"		\
			"378b2o94bobo$180b2o292b2o$180bobo$180bo4$43b2o98b2o124b2o72b2o124b2o\n"	\
			"98b2o$43b2o98b2o124b2o72b2o124b2o98b2o6$95bobo$95b2o$96bo3$521bobo$\n"		\
			"522b2o$522bo4$35b2o114b2o108b2o88b2o108b2o114b2o$35b2o114b2o108b2o88b\n"	\
			"2o108b2o114b2o12$43b2o224b2o72b2o224b2o$43b2o224b2o72b2o224b2o17$35b2o\n"	\
			"114b2o108b2o88b2o108b2o114b2o$35b2o114b2o108b2o88b2o108b2o114b2o12$43b\n"	\
			"2o524b2o$43b2o524b2o17$35b2o114b2o108b2o88b2o108b2o114b2o$35b2o114b2o\n"	\
			"108b2o88b2o108b2o114b2o!\n"

#define TRAIL_RLE	"x = 544, y = 2, rule = B3/S23\n"						\
			"o40b2o114b2o108b2o88b2o108b2o114b2o$41b2o114b2o108b2o88b2o108b2o114b2o!\n"



typedef struct
  {
    ROWID rId, tId, bId;
    int   offX, offY;
    int   lane;
    int   delta;
    int   nph;
    int   delay;
  } trace;


ROWID tIds [P_MAX];
int   n_tIds = 0;
trace recipe [MAXPARTS];
int   n_recipe = 0;
MYSQL *con;
bullet bullets [1];

pattern rephaser;
int rephaser_X;
pattern R1L0 [P_MAX];
int R1L0_X [P_MAX];
pattern result;
pattern trail;


static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%s\n", mysql_error (con));
  mysql_close (con);
  exit (2);
}


static void load_tgt_bbox (target *t, ROWID tId, int offX, int offY)

{
  char query [4096];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int width, height;

  snprintf (query, 4095, "SELECT combined_width, combined_height FROM target WHERE tId = %llu", tId);
  if (mysql_query (con, query))
    finish_with_error (con);

  result = mysql_store_result (con);
  if (!result)
    return;

  row = mysql_fetch_row (result);
  if (!row)
    return;

  width = strtoull (row [0], NULL, 0);
  height = strtoull (row [1], NULL, 0);
  mysql_free_result (result);

  t->top = offY;
  t->left = offX;
  t->bottom = t->top + height - 1;
  t->right = t->left + width - 1;

  // printf ("bbox[%llu]: top=%d, left=%d, bottom=%d, right=%d\n", tId, t->top, t->left, t->bottom, t->right);
}


static void load_targets_ids (ROWID tId)
// Load the tId values for all phases of the target defined by given tId

{
  char query [4096];
  MYSQL_RES *result;
  MYSQL_ROW row;

  // We already now one tId keep.
  tIds [0] = tId;
  n_tIds = 1;

  while (true)
    {
      snprintf (query, 4095, "SELECT next_tId FROM target WHERE tId = %llu", tId);
      if (mysql_query (con, query))
	finish_with_error (con);

      result = mysql_store_result (con);
      if (!result)
	break;

      row = mysql_fetch_row (result);
      if (!row || !row [0])
	break;

      tId = strtoull (row [0], NULL, 0);
      mysql_free_result (result);

      if (!tId || tId == tIds [0])
	break;

      tIds [n_tIds++] = tId;
      if (n_tIds >= 100)
	break;
    }
}


static int cost_for (int old_lane, int new_lane)

{
  int d = new_lane - old_lane;

  // C operator % does not what we want, if LHS < 0
  while (d < 0)
    d += nCOSTS;

  return COSTS [d % nCOSTS];
}


static void load_bullet (bullet *b, ROWID bId)

{
  char query [4096];
  MYSQL_RES *result;
  MYSQL_ROW row;

  b->id = bId;

  snprintf (query, 4095, "SELECT b.oId, rle, dx, dy, dt, base_x, base_y, reference, lane_dx, lane_dy, lanes_per_height, lanes_per_width, extra_lanes "
				"FROM bullets b LEFT JOIN objects USING (oId) WHERE bId = %llu", b->id);

  if (mysql_query (con, query))
    finish_with_error (con);

  result = mysql_store_result (con);
  if (!result)
    return;

  row = mysql_fetch_row (result);
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
  mysql_free_result (result);
}


static ROWID parent_reaction_of (ROWID rId)
// search reaction which not only yields the initial target for reaction rId as a result, but also has the correct cost.

{
  char query [4096];
  MYSQL_RES *result;
  MYSQL_ROW row;
  ROWID ret = 0, i_tId, r_tId, tId;
  int   cost, new_lane;
  target old, new;
 
  snprintf (query, 4095, "SELECT initial_tId, lane, cost FROM reaction WHERE rId = %llu", rId);
  if (mysql_query (con, query))
    finish_with_error (con);

  result = mysql_store_result (con);
  if (!result)
    return ret;

  row = mysql_fetch_row (result);
  if (!row)
    return ret;

  tId = strtoull (row [0], NULL, 0);
  new_lane = atoi (row [1]);
  cost = atoi (row [2]);
  mysql_free_result (result);

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

  result = mysql_store_result (con);
  if (!result)
    return ret;

  while (!ret && (row = mysql_fetch_row(result)))
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

  mysql_free_result (result);

  return ret;
}

 
main (int argc, char **argv)

{
  int i, j, n, xPos, yPos, delay;

  bullets [0].id = 0;

  config_load (argv [1]);
assert (MAXPERIOD <= P_MAX);

  con = mysql_init(NULL);
  if (mysql_real_connect (con, DBHOST, DBUSER, DBPASSWD, DBNAME, DBPORT, NULL, 0) == NULL)
    finish_with_error (con);

  // Make room for our the ship parts
  MAXWIDTH = PARTWIDTH;
  MAXHEIGHT = PARTHEIGHT;
  MAXGEN = MAXPERIOD;
  MAX_RLE = 65536;
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);
  pat_allocate (&result, PARTWIDTH, PART_DY*MAXPARTS+PARTHEIGHT);

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

  for (i = 2; i < argc; i++)
    {
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

      pat_init (&result);
      yPos = 0;
      pat_copy (&result, xPos-R1L0_X [0], yPos, &R1L0 [0]);
//printf ("R1L0 [0]: %d = %d-%d\n", xPos-R1L0_X [0], xPos, R1L0_X [0]);
      yPos += PART_DY;
      --n_recipe;
      printf ("# Start with reaction %llu\n", recipe [n_recipe].rId);
      while (n_recipe > 0)
	{
	  assert (recipe [n_recipe].bId == 1);
	  delay = (delay + recipe [n_recipe].delay) % recipe [n_recipe].nph;
	  if (recipe [n_recipe].delta > 1) printf ("# REPHASE x%d\n", recipe [n_recipe].delta-1);
	  if (delay) printf ("# DELAY %dgen\n", delay);
	  // printf ("# FIRE 1RL0\n");
	  for (j = 0; j < recipe [n_recipe].delta-1; j++)
	    {
	      pat_add (&result, xPos-rephaser_X, yPos, &rephaser);
// printf ("rephase: %d = %d-%d\n", xPos-rephaser_X, xPos, rephaser_X);
	      yPos += PART_DY;
	    }
	  // TO DO: carry delay over to next part!
	  pat_add (&result, xPos-R1L0_X [delay], yPos, &R1L0 [delay]);
// printf ("R1L0 [%d]: %d = %d-%d\n", j, xPos-R1L0_X [j], xPos, R1L0_X [j]);
	  yPos += PART_DY;
	  n_recipe--;
	  printf ("# FIRE 1RL0 [resulting reaction:%llu]\n", recipe [n_recipe].rId);
	}

      for (j = 0; j < 2000; j++)
	{
	  int k;
	  pat_dump (&trail, false);
	  for (k = 0; k < 29; k++)
	    printf ("..\n");
	}
      pat_dump (&result, false);
      // printf ("x = %d, y = %d, rule = B3/S23\n%s\n", W(&result), H(&result), pat_rle (&result));
    }
}
