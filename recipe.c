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


typedef struct
  {
    ROWID rId, tId, bId;
    int   offX, offY;
    int   lane;
    int   delta;
    int   shift;
  } trace;


ROWID tIds [100];
int   n_tIds = 0;
trace recipe [100];
int   n_recipe = 0;
MYSQL *con;
bullet bullets [1];


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

printf ("old: [top=%d, bottom=%d, left=%d, right=%d]\n", old.top, old.bottom, old.left, old.right);
printf ("new: [top=%d, bottom=%d, left=%d, right=%d]\n", new.top, new.bottom, new.left, new.right);
printf ("old_lane=%d, new_lane=%d, new_Lane_adj=%d\n", old_lane, new_lane, new_lane-tgt_adjust_lane (0, &old, &new), new_lane);
printf ("correct cost: %d\n", cost_for (old_lane, new_lane-tgt_adjust_lane (0, &old, &new)));

      // printf ("old=%d, adj=%d, new=%d\n", old_lane, tgt_adjust_lane (0, &old, &new), new_lane);
      // printf ("%llu: %d ./. %d\n", toChk, delta, cost_for (old_lane - tgt_adjust_lane (0, &old, &new), new_lane));
      if (delta == cost_for (old_lane + tgt_adjust_lane (0, &old, &new), new_lane))
	{
	  ret = toChk;
	  recipe [n_recipe  ].rId = toChk;
	  recipe [n_recipe  ].tId = i_tId;
	  recipe [n_recipe  ].bId = bId;
	  recipe [n_recipe  ].offX = offX;
	  recipe [n_recipe  ].offY = offY;
	  recipe [n_recipe  ].lane = old_lane - tgt_adjust_lane (0, &old, &new);
	  recipe [n_recipe  ].delta = delta;
	  recipe [n_recipe++].shift = 47*(r_tId != tId);
	}
    }

  mysql_free_result (result);

  return ret;
}

 
main (int argc, char **argv)

{
  int i, n;

  bullets [0].id = 0;

  config_load (argv [1]);

  con = mysql_init(NULL);
  if (mysql_real_connect (con, DBHOST, DBUSER, DBPASSWD, DBNAME, DBPORT, NULL, 0) == NULL)
    finish_with_error (con);

  // Initialize all modules.
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);

  for (i = 2; i < argc; i++)
    {
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

      --n_recipe;
      printf ("Start with reaction %llu\n", recipe [n_recipe].rId);
      while (n_recipe > 0)
	{
assert (recipe [n_recipe].bId == 1);
	  if (recipe [n_recipe].shift) printf ("DELAY 1gen [NYI:count]\n");
	  if (recipe [n_recipe].delta > 1) printf ("REPHASE x%d\n", recipe [n_recipe].delta-1);
	  printf ("FIRE 1RL0\n");
	  n_recipe--;
	}
    }
}
