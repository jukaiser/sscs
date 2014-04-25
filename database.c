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

MYSQL *con;

void finish_with_error(MYSQL *con)

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
      exit (2); }

  if (mysql_real_connect (con, DBHOST, DBUSER, DBPASSWD, DBNAME, DBPORT, NULL, 0) == NULL)
    finish_with_error (con);
}


ROWID db_target_lookup (uint32_t hash, const char *rle)

{
  ROWID ret = 0ULL;
  char query [4096];
  snprintf (query, 4095, SQL_F_SEARCH_TARGET, hash, rle);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  MYSQL_ROW row = 0;

  if (result)
    row = mysql_fetch_row (result);
  else if (mysql_errno (con))
    finish_with_error (con);

  if (row)
    ret = strtoull (row [0], NULL, 0);
  mysql_free_result (result);

  return ret;
}


ROWID db_target_store (uint32_t hash, const char *rle, unsigned nph, unsigned width, unsigned height)
// Lookup target given by (hash, rle) - if not found, store it.

{
  int ret;
  char query [4096];

  ret = db_target_lookup (hash, rle);
  if (ret)
    return ret;

  snprintf (query, 4095, SQL_F_STORE_TARGET, hash, rle, nph, width, height);

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


part *db_load_parts_for (const char *shipname, bool complete, unsigned nr_phases, int baseX, int baseY)
// Load the parts we could use for the given SHIP.

{
  part *ret;
  char query [4096];
  int n, i, p;

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

  snprintf (query, 4095, complete ? SQL_F_PARTS_COMPLETE : SQL_F_PARTS, shipname);
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

      // Does our caller really want all them fields? Especially the rle/pattern??
      if (complete)
	{
	  ALLOC(target, ret [i].pats, nr_phases+1);
	  pat_from_string (row [7]);
	  ret [i].dx = atoi (row [8]);
	  ret [i].dy = atoi (row [9]);
	  ret [i].pats [0].pat = pat_compact (&lab [0], NULL);
	  ret [i].pats [0].X = baseX + atoi (row [10]);
	  ret [i].pats [0].Y = baseY + atoi (row [11]);
	  if (ret [i].type == pt_rake)
	    {
	      pat_fill (&lab [0], DEAD);
	      pat_copy (&lab [0], ret [i].pats [0].X, ret [i].pats [0].Y, ret [i].pats [0].pat);
	      for (p = 0; p < nr_phases; p++)
		{
		  pat_generate (&lab [p], &lab [p+1]);
		  ret [i].pats [p+1].pat = pat_compact (&lab [p+1], NULL);
		  ret [i].pats [p+1].X = lab [p+1].left;
		  ret [i].pats [p+1].Y = lab [p+1].top;
		}
	      ret [i].fireX = baseX + atoi (row [12]);
	      ret [i].fireY = baseY + atoi (row [13]);
	    }
	}
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


ROWID db_is_reaction_finished (ROWID tId, unsigned phase, unsigned b, unsigned lane, unsigned *cost)

{
  char query [4096];
  ROWID ret = 0ULL;

  snprintf (query, 4095, SQL_F_IS_FINISHED_REACTION, tId, phase, bullets [b].id, lane);

  if (__dbg_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  MYSQL_ROW row = 0;

  if (result)
    row = mysql_fetch_row (result);
  else if (mysql_errno (con))
    finish_with_error (con);

  if (row && row [0])
    {
      ret = strtoull (row [0], NULL, 0);
      *cost = atoi (row [1]);
    }

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

  snprintf (query, 4095, SQL_F_STORE_REACTION, r->tId, r->phase, bullets [r->b].id, r->lane, res->lane_adj, res->result_tId, res->result_phase,
	    res->offX, res->offY, res->delay, res->gen, r->cost, t, (res->emits?"true":"false"));

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


void  db_store_emit (ROWID rId, unsigned seq, ROWID oId, int offX, int offY, int gen)

{
  char query [4096];

  assert (rId);
  snprintf (query, 4095, SQL_F_STORE_EMIT, rId, seq, oId, offX, offY, gen);

  if (__dbg_query (con, query))
    finish_with_error (con);
}


unsigned db_target_fetch (ROWID tId)

{
  char query [4096];
  int width, height, offX, offY;
  unsigned ret;

  snprintf (query, 4095, SQL_F_FETCH_TARGET, tId);

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
  ret = atoi (row [1]);

  mysql_free_result (result);

  return ret;
}


void db_store_transition (ROWID rId, unsigned old_state, unsigned new_state, unsigned rephase, ROWID pId, unsigned total_cost, unsigned delta_cost)

{
  char query [4096];

  snprintf (query, 4095, SQL_F_STORE_TRANSITION, rId, old_state, new_state, rephase, pId, delta_cost, total_cost);

  if (__dbg_query (con, query))
    {
      if (mysql_errno (con) != ER_DUP_ENTRY)
	finish_with_error (con);
    }
}


void db_target_reload (target *tgt, ROWID tId, unsigned phase)
// Helperfunction: we have loaded phase 0 from the db into lab [0].
// We need to reconstruct the given phase and all other infos from that.

{
  int i, nph;
  int left, right, top, bottom;

  nph = db_target_fetch (tId);

  assert (phase < nph);

  // Build other phases.
  for (i = 1; i < nph; i++)
    pat_generate (&lab [i-1], &lab [i]);

  // calculate common bbox of all phases of current target.
  top    = lab [0].top;
  bottom = lab [0].bottom;
  left   = lab [0].left;
  right  = lab [0].right;
  for (i = 1; i < nph; i++)
    {
      if (lab [i].top    < top)    top    = lab [i].top;
      if (lab [i].bottom > bottom) bottom = lab [i].bottom;
      if (lab [i].left   < left)   left   = lab [i].left;
      if (lab [i].right  > right)  right  = lab [i].right;
    }

  // Build target.
  tgt->pat = pat_compact (&lab [phase], NULL);
  tgt->id = tId,
  tgt->phase = phase;
  tgt->nph = nph;
  tgt->top = top;
  tgt->bottom = bottom;
  tgt->left = left;
  tgt->right = right;
  tgt->X = lab [phase].left;
  tgt->Y = lab [phase].top;
}
