/* Database driver - replace mysql/mariadb with your DB, if you like */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <mysqld_error.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"
#include "profile.h"


#ifdef NO_PROFILING
#define __dbg_query mysql_query
#endif

static MYSQL *con;

static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%u: %s\n", mysql_errno (con), mysql_error (con));
  mysql_close (con);
  exit (2);
}


#ifndef __dbg_query
int __dbg_query(MYSQL *mysql, const char *q)

{
  int ret;
  struct timespec t, t1, t2;
  t.tv_sec = t.tv_nsec = 0;

  _prof_enter ();
  ret = mysql_query (mysql, q);
  _prof_leave (q);

  return ret;
}
#endif


void db_init (void)

{
  con = mysql_init(NULL);

  // make sure our notion of ROWID will work on this platform.
  // YMWV
  if (sizeof (ROWID) != sizeof (my_ulonglong))
    {
      fprintf (stderr, "Please redefine ROWID in pattern.h to fit my_ulonglong on your platform!\nThen you have to recompile\n");
      exit (3);
    }

  if (con == NULL)
    {
      fprintf(stderr, "mysql_init () failed\n");
      exit (2);
    }

  if (mysql_real_connect (con, DBHOST, DBUSER, DBPASSWD, DBNAME, DBPORT, NULL, 0) == NULL)
    finish_with_error (con);
}


bool db_target_lookup (target *tgt, const char *rle)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_SEARCH_TARGET, rle);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  MYSQL_ROW row = 0;

  if (result)
    row = mysql_fetch_row (result);
  else if (mysql_errno (con))
    finish_with_error (con);

  if (row)
    tgt->id = strtoull (row [0], NULL, 0);
  else
    tgt->id = 0;
  mysql_free_result (result);

  return (tgt->id != 0);
}


void db_target_store (target *tgt, const char *rle, ROWID prev)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_STORE_TARGET, rle, W(tgt->pat), H(tgt->pat), W(tgt), H(tgt), tgt->X-tgt->left, tgt->Y-tgt->top, tgt->nph, prev);

  if (__dbg_query (con, query))
    {
      if (mysql_errno (con) != ER_DUP_ENTRY)
	finish_with_error (con);
      else
	{
	  db_target_lookup (tgt, rle);
	  return;
	}
    }
  if (mysql_affected_rows (con) != 1)
    {
      db_target_lookup (tgt, rle);
      return;
    }

  tgt->id = mysql_insert_id(con);
}


void db_target_link (ROWID curr, ROWID nxt)

{
assert (0); // KILL IT WITH FIRE
}


bullet *db_load_bullets_for (const char *shipname)

{
  bullet *ret;
  char query [4096];
  int n, i;

  snprintf (query, 4095, SQL_COUNT_BULLETS, shipname);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "No bullets found for '%s'\n", shipname);
      exit (2);
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  n = strtoull (row [0], NULL, 0);
  mysql_free_result (result);

  if (n < 1)
    {
      fprintf (stderr, "No bullets found for '%s'\n", shipname);
      exit (2);
    }

  ALLOC(bullet,ret,n+1)

  snprintf (query, 4095, SQL_F_BULLETS, shipname);
  if (__dbg_query (con, query))
    finish_with_error (con);

  result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "No bullets found for '%s'\n", shipname);
      exit (2);
    }

  for (i = 0; i < n; i++)
    {
      row = mysql_fetch_row (result);

      if (!row)
	{
	  fprintf (stderr, "Please check database! SQL_COUNT_BULLETS ./. SQL_F_BULLETS?");
	  exit (2);
	}

      ret [i].id = strtoull (row [0], NULL, 0);
      ret [i].oId = strtoull (row [1], NULL, 0);
      pat_from_string (row [2]);
      ret [i].p = pat_compact (&lab [0], NULL);
      ret [i].dx = atoi (row [3]);
      ret [i].dy = atoi (row [4]);
      ret [i].dt = atoi (row [5]);
      ret [i].base_x = atoi (row [6]);
      ret [i].base_y = atoi (row [7]);
      if (strcmp (row [8], "TOPLEFT") == 0)
	ret [i].reference = topleft;
      else if (strcmp (row [8], "TOPRIGHT") == 0)
	ret [i].reference = topright;
      else if (strcmp (row [8], "BOTTOMLEFT") == 0)
	ret [i].reference = bottomleft;
      else if (strcmp (row [8], "BOTTOMRIGHT") == 0)
	ret [i].reference = bottomright;
      ret [i].lane_dx = atoi (row [9]);
      ret [i].lane_dy = atoi (row [10]);
      ret [i].lanes_per_height = atoi (row [11]);
      ret [i].lanes_per_width = atoi (row [12]);
      ret [i].extra_lanes = atoi (row [13]);
    }


  ret [n].id = 0;
  ret [n].oId = 0;
  ret [n].p = NULL;

  mysql_free_result (result);

  return ret;
}


part *db_load_parts_for (const char *shipname)
// Load the parts we could use for the given SHIP.

{
  part *ret;
  char query [4096];
  int n, i;

  snprintf (query, 4095, SQL_COUNT_PARTS, shipname);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "No parts found for '%s'\n", shipname);
      exit (2);
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  n = strtoull (row [0], NULL, 0);
  mysql_free_result (result);

  if (n < 1)
    {
      fprintf (stderr, "No parts found for '%s'\n", shipname);
      exit (2);
    }

  ALLOC(part,ret,n+1)

  snprintf (query, 4095, SQL_F_PARTS, shipname);
  if (__dbg_query (con, query))
    finish_with_error (con);

  result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "No parts found for '%s'\n", shipname);
      exit (2);
    }

  for (i = 0; i < n; i++)
    {
      row = mysql_fetch_row (result);

      if (!row)
	{
	  fprintf (stderr, "Please check database! SQL_COUNT_PARTS ./. SQL_F_PARTS?");
	  exit (2);
	}

      ret [i].pId = strtoull (row [0], NULL, 0);
      ret [i].name = strdup (row [1]); if (!ret [i].name) { perror ("db_load_space_ships - strdup"); exit (2); }
      if (strcmp (row [2], "track") == 0)
	ret [i].type = pt_track;
      else if (strcmp (row [2], "rephaser") == 0)
	ret [i].type = pt_rephaser;
      else if (strcmp (row [2], "rake") == 0)
	ret [i].type = pt_rake;
      ret [i].lane_adjust = atoi (row [3]);
      if (ret [i].type == pt_rake)
	{
	  ret [i].b = bullet_index (atoi (row [4]));
	  ret [i].lane_fired = atoi (row [5]);
	}
      else
	ret [i].b = -1;
      ret [i].cost = atoi (row [6]);
    }


  ret [n].pId = 0;
  ret [n].name = NULL;

  mysql_free_result (result);

  return ret;
}


object *db_load_space_ships (void)
// Load ALL space ships from our database

{
  object *ret;
  int i, n, j;

  if (__dbg_query (con, SQL_COUNT_SPACESHIPS))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "Please check database! No ships found with SQL_F_COUNT_SPACESHIPS?");
      exit (2);
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  n = strtoull (row [0], NULL, 0);
  mysql_free_result (result);

  if (n < 1)
    {
      fprintf (stderr, "Please check database! No ships found with SQL_F_COUNT_SPACESHIPS?");
      exit (2);
    }

  ALLOC(object,ret,n+1)

  if (__dbg_query (con, SQL_F_SPACESHIPS))
    finish_with_error (con);

  result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "Please check database! No ships found with SQL_F_SPACESHIPS?");
      exit (2);
    }

  for (i = 0; i < n; i++)
    {
      row = mysql_fetch_row (result);

      if (!row)
	{
	  fprintf (stderr, "Please check database! SQL_F_COUNT_SPACESHIPS ./. SQL_F_SPACESHIPS?");
	  exit (2);
	}

      ret [i].id = strtoull (row [0], NULL, 0);
      pat_from_string (row [1]);
      ret [i].compact = pat_compact (&lab [0], NULL);
      pat_envelope (&lab [0]);
      ret [i].enveloped = pat_compact (&lab [0], NULL);
      ret [i].name = strdup (row [2]); if (!ret [i].name) { perror ("db_load_space_ships - strdup"); exit (2); }
      ret [i].dx = atoi (row [3]);
      ret [i].dy = atoi (row [4]);
      ret [i].dt = atoi (row [5]);
      ret [i].phase = atoi (row [6]);
      if (ret [i].phase == 0)
	ret [i].base = &ret [i];
      else
	 ret [i].base = ret [i-1].base;
assert (&ret [i] == (ret [i].base + ret [i].phase));
      ret [i].offX = atoi (row [7]);
      ret [i].offY = atoi (row [8]);
      obj_mark_first (&ret [i]);
      ret [i].wanted = false;
      for (j = 0; j < nSHIP_DIRS; j++)
	switch ((ship_dirs) SHIP_DIRS [j])
	  {
	    case dir_north:
	      if (ret [i].dy < 0 && ret [i].dx == 0)
		ret [i].wanted = true;
	      break;
	    case dir_northeast:
	      if (ret [i].dy < 0 && ret [i].dx > 0)
		ret [i].wanted = true;
	      break;
	    case dir_east:
	      if (ret [i].dy == 0 && ret [i].dx > 0)
		ret [i].wanted = true;
	      break;
	    case dir_southeast:
	      if (ret [i].dy > 0 && ret [i].dx > 0)
		ret [i].wanted = true;
	      break;
	    case dir_south:
	      if (ret [i].dy > 0 && ret [i].dx == 0)
		ret [i].wanted = true;
	      break;
	    case dir_southwest:
	      if (ret [i].dy > 0 && ret [i].dx < 0)
		ret [i].wanted = true;
	      break;
	    case dir_west:
	      if (ret [i].dy == 0 && ret [i].dx < 0)
		ret [i].wanted = true;
	      break;
	    case dir_north_west:
	      if (ret [i].dy < 0 && ret [i].dx < 0)
		ret [i].wanted = true;
	      break;
	  }
    }

  ret [n].id = 0;
  ret [n].enveloped = NULL;
  ret [n].compact = NULL;
  ret [n].name = NULL;

  mysql_free_result (result);

  return ret;
}


bool db_is_reaction_finished (ROWID tId, unsigned phase, unsigned b, unsigned lane)

{
  char query [4096];
  bool ret;

  snprintf (query, 4095, SQL_F_IS_FINISHED_REACTION, tId, phase, bullets [b].id, lane);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  MYSQL_ROW row = 0;

  if (result)
    row = mysql_fetch_row (result);
  else if (mysql_errno (con))
    finish_with_error (con);

  if (row)
    ret = (row [0] && row [0][0]);
  else
    ret = false;

// if (ret) printf ("INFO: Skipping already handled reaction tId=%llu ./. (bullet=%u,lane=%u) [stored result was '%s']\n", tId, b, lane, row [0]);

  mysql_free_result (result);

  return ret;
}


ROWID db_reaction_keep (reaction *r, result *res)
// sync the given reaction with our database.
// return false if we do not need to handle it anymore.
// TO DO: we should take a closer look at "exploding" and "unfinished" reactions. Maybe the context has change since the last run ...

{
  char query [4096];
  bool ret;
  char *t;

  switch (res->type)
    {
      case rt_undef:
	t = "";
	break;
      case rt_dies:
	t = "dies";
	break;
      case rt_flyby:
	t = "fly-by";
	break;
      case rt_stable:
	t = "stable";
	break;
      case rt_pruned:
	t = "pruned";
	break;
      case rt_unfinished:
	t = "unfinished";
	break;
    }

  snprintf (query, 4095, SQL_F_STORE_REACTION, r->tId, bullets [r->b].id, r->lane, res->result_tId, res->offX, res->offY, res->gen, t, r->cost, (res->emits?"true":"false"));

  if (__dbg_query (con, query))
    {
      if (mysql_errno (con) != ER_DUP_ENTRY)
	finish_with_error (con);
      else
	return 0ULL;
    }
  if (mysql_affected_rows (con) != 1)
    return 0ULL;

  return mysql_insert_id (con);
}


void  db_store_emit (ROWID rId, ROWID oId, int offX, int offY, int gen)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_STORE_EMIT, rId, oId, offX, offY, gen);

  if (__dbg_query (con, query))
    finish_with_error (con);

#if 0
  db_reaction_emits (rId);
#endif
}


void db_reaction_finish (reaction *r, ROWID result_tId, int offX, int offY, int gen, reaction_type type)

{
assert (0);
#if 0
  char query [4096];
  char *t;

  switch (type)
    {
      case rt_undef:
	t = "";
	break;
      case rt_dies:
	t = "dies";
	break;
      case rt_flyby:
	t = "fly-by";
	break;
      case rt_stable:
	t = "stable";
	break;
      case rt_pruned:
	t = "pruned";
	break;
      case rt_unfinished:
	t = "unfinished";
	break;
    }

  snprintf (query, 4095, SQL_F_FINISH_REACTION, result_tId, offX, offY, gen, t, r->cost, r->rId);

  if (__dbg_query (con, query))
    finish_with_error (con);
#endif
}


void db_target_fetch (reaction *r, target *t)

{
  char query [4096];
  int width, height, offX, offY;

  assert (r);
  snprintf (query, 4095, SQL_F_FETCH_TARGET, r->tId);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  if (!result)
    {
      fprintf (stderr, "Database error: target not found. Query = '%s'\n", query);
      exit (2);
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  if (!row)
    {
      fprintf (stderr, "Database error: target not found. Query = '%s'\n", query);
      exit (2);
    }

  pat_from_string (row [0]);
  t->pat = pat_compact (&lab [0], NULL);
  t->nph = atoi (row [1]);
  width = atoi (row [2]);
  height = atoi (row [3]);
  offX = atoi (row [4]);
  offY = atoi (row [5]);
  mysql_free_result (result);
  t->X = lab [0].left;
  t->Y = lab [0].top;
  t->top = t->Y - offY;
  t->bottom = t->top + height - 1;
  t->left = t->X - offX;
  t->right = t->left + width - 1;
}
