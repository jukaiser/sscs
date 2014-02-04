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


/*
typedef struct
  {
  } bullet;
*/


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
  pat_from_string (row [0]);
  b->p = pat_compact (&lab [0], NULL);
  b->dx = atoi (row [1]);
  b->dy = atoi (row [2]);
  b->dt = atoi (row [3]);
  b->base_x = atoi (row [4]);
  b->base_y = atoi (row [5]);
  if (strcmp (row [6], "TOPLEFT") == 0)
    b->reference = topleft;
  else if (strcmp (row [6], "TOPRIGHT") == 0)
    b->reference = topright;
  else if (strcmp (row [6], "BOTTOMLEFT") == 0)
    b->reference = bottomleft;
  else if (strcmp (row [6], "BOTTOMRIGHT") == 0)
    b->reference = bottomright;
  b->lane_dx = atoi (row [7]);
  b->lane_dy = atoi (row [8]);
  b->lanes_per_width = atoi (row [9]);
  b->lanes_per_height = atoi (row [10]);
  b->extra_lanes = atoi (row [11]);

  mysql_free_result (result);

  return b;
}
