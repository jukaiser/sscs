/* Database driver - replace mysql/mariadb with your DB, if you like */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "pattern.h"
#include "guarded_ptr.h"
#include "reaction.h"
#include "database.h"


static MYSQL *con;

static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%s\n", mysql_error (con));
  mysql_close (con);
  exit (2);
}


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

  if (mysql_real_connect (con, "localhost", "gol", "GoL", "gol", 0, NULL, 0) == NULL)
    finish_with_error (con);
}


static bool db_target_lookup (target *tgt, const char *rle)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_SEARCH_TARGET, rle);

  if (mysql_query (con, query))
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


static void db_target_store (target *tgt, const char *rle)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_STORE_TARGET, rle, W(tgt->pat), H(tgt->pat), W(tgt), H(tgt), tgt->X-tgt->left, tgt->Y-tgt->top, tgt->nph);

  if (mysql_query (con, query))
    finish_with_error (con);

  tgt->id = mysql_insert_id(con);
}


static void db_target_link (ROWID curr, ROWID nxt)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_LINK_TARGET,  nxt, curr);

  if (mysql_query (con, query))
    finish_with_error (con);
}


bullet *db_bullet_load (const char *name)

{
  bullet *b = malloc (sizeof (bullet));
  if (!b)
    {
      perror ("db_bullet_load - malloc");
      exit (2);
    }

  char query [4096];
  snprintf (query, 4095, SQL_F_BULLET, name);

  if (mysql_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (!result)
    {
      free (b);
      return NULL;
    }

  MYSQL_ROW row = mysql_fetch_row (result);
  b->id = strtoull (row [0], NULL, 0);
  b->name = strdup (name); if (!b->name) { perror ("db_bullet_load - strdup"); exit (2); }
  b->oId = strtoull (row [1], NULL, 0);
  pat_from_string (row [2]);
  b->p = pat_compact (&lab [0], NULL);
  b->dx = atoi (row [3]);
  b->dy = atoi (row [4]);
  b->dt = atoi (row [5]);
  b->base_x = atoi (row [6]);
  b->base_y = atoi (row [7]);
  if (strcmp (row [8], "TOPLEFT") == 0)
    b->reference = topleft;
  else if (strcmp (row [8], "TOPRIGHT") == 0)
    b->reference = topright;
  else if (strcmp (row [8], "BOTTOMLEFT") == 0)
    b->reference = bottomleft;
  else if (strcmp (row [8], "BOTTOMRIGHT") == 0)
    b->reference = bottomright;
  b->lane_dx = atoi (row [9]);
  b->lane_dy = atoi (row [10]);
  b->lanes_per_height = atoi (row [11]);
  b->lanes_per_width = atoi (row [12]);
  b->extra_lanes = atoi (row [13]);

  mysql_free_result (result);

  return b;
}


object *db_load_space_ships (void)
// Load ALL space ships from our database

{
  object *ret;
  int i, n;

  if (mysql_query (con, SQL_COUNT_SPACESHIPS))
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

  ret = calloc (n+1, sizeof (object));
  if (!ret)
    {
      perror ("db_load_space_ships - calloc");
      exit (2);
    }

  if (mysql_query (con, SQL_SPACESHIPS))
    finish_with_error (con);

  result = mysql_store_result (con);

  if (!result)
    {
      fprintf (stderr, "Please check database! No ships found with SQL_F_COUNT_SPACESHIPS?");
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
      pat_envelope (&lab [0]);
      ret [i].pat = pat_compact (&lab [0], NULL);
      ret [i].name = strdup (row [2]); if (!ret [i].name) { perror ("db_load_space_ships - strdup"); exit (2); }
      ret [i].dx = atoi (row [3]);
      ret [i].dy = atoi (row [4]);
      ret [i].dt = atoi (row [5]);
      obj_mark_first (&ret [i]);
    }

  ret [n].id = 0;
  ret [n].pat = NULL;
  ret [n].name = NULL;

  mysql_free_result (result);

  return ret;
}


void db_targets_keep (target **tgts, int nph)

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


bool db_is_reaction_finished (reaction *r)
// TO DO: we should take a closer look at "exploding" and "unfinished" reactions. Maybe the context has change since the last run ...

{
  char query [4096];
  target *tgt = (target *) r->g_tgt->ptr;
  bool ret;

  snprintf (query, 4095, SQL_F_IS_FINISHED_REACTION, r->id);

  if (mysql_query (con, query))
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
  mysql_free_result (result);

  return ret;
}


bool db_reaction_keep (reaction *r)
// sync the given reaction with our database.
// return false if we do not need to handle it anymore.
// TO DO: we should take a closer look at "exploding" and "unfinished" reactions. Maybe the context has change since the last run ...

{
  char query [4096];
  target *tgt = (target *) r->g_tgt->ptr;
  bool ret;

  snprintf (query, 4095, SQL_F_FETCH_REACTION, tgt->id, r->bullet->id, r->lane);

  if (mysql_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);
  MYSQL_ROW row = 0;

  if (result)
    row = mysql_fetch_row (result);
  else if (mysql_errno (con))
    finish_with_error (con);

  if (row)
    {
      r->id = strtoull (row [0], NULL, 0);
      ret = (!row [1] || !row [1][0]);
    }
  else
    r->id = 0;
  mysql_free_result (result);

  if (r->id)
    return ret;

  snprintf (query, 4095, SQL_F_STORE_REACTION, tgt->id, r->bullet->id, r->lane);

  if (mysql_query (con, query))
    finish_with_error (con);

  r->id = mysql_insert_id(con);
  return true;	// new reaction means: definetly not handled!
}


void  db_store_emit (ROWID rId, ROWID oId, int offX, int offY, int gen)

{
  char query [4096];
  snprintf (query, 4095, SQL_F_STORE_EMIT, rId, oId, offX, offY, gen);

  if (mysql_query (con, query))
    finish_with_error (con);

  snprintf (query, 4095, SQL_F_REACTION_EMITS, rId);

  if (mysql_query (con, query))
    finish_with_error (con);
}


void db_reaction_finish (reaction *r, ROWID result_tId, int offX, int offY, int gen, db_reaction_type type)

{
  char query [4096];
  char *t;

  switch (type)
    {
      case dbrt_undef:
	t = "";
	break;
      case dbrt_dies:
	t = "dies";
	break;
      case dbrt_flyby:
	t = "fly-by";
	break;
      case dbrt_stable:
	t = "stable";
	break;
      case dbrt_unfinished:
	t = "unfinished";
	break;
    }

  snprintf (query, 4095, SQL_F_FINISH_REACTION, result_tId, offX, offY, gen, t, r->id);

  if (mysql_query (con, query))
    finish_with_error (con);
}

