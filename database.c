/* Database driver - replace mysql/mariadb with your DB, if you like */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "pattern.h"
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


bool db_target_exists (const char *rle, int sizeX, int sizeY)

{
  bool res = false;
  char query [4096];
  snprintf (query, 4095, "SELECT tId FROM target WHERE rle = '%s'", rle);

  if (mysql_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (result && mysql_fetch_row (result))
    res = true;
  else if (mysql_errno (con))
    finish_with_error (con);

  mysql_free_result (result);

  return res;
}


void db_target_insert (const char *rle, int sizeX, int sizeY)

{
  char query [4096];
  snprintf (query, 4095, "INSERT INTO target (tId, rle, x, y) VALUES (NULL, '%s', %d, %d)", rle, sizeX, sizeY);

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
  b->lanes_per_width = atoi (row [10]);
  b->lanes_per_height = atoi (row [11]);
  b->extra_lanes = atoi (row [12]);

  mysql_free_result (result);

  return b;
}

/*
    ROWID    id;                        // ROW ID in database table (0 if not yet stored!)
    pattern *pat;
    char    *name;
    int      dx, dy, dt;                // speed and period of the bullet
*/

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

