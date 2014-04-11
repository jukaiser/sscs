/* main body of reaction visualizer */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>
#include <stdint.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"


static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%s\n", mysql_error (con));
  mysql_close (con);
  exit (2);
}


main (int argc, char **argv)

{
  char query [4096];
  bullet b;
  int lane;
  target t;
  int i, dummy, offX, offY, width, height;
  MYSQL_RES *result;
  MYSQL_ROW row;

  config_load (argv [1]);

  MYSQL *con = mysql_init(NULL);
  if (mysql_real_connect (con, DBHOST, DBUSER, DBPASSWD, DBNAME, DBPORT, NULL, 0) == NULL)
    finish_with_error (con);

  // Initialize all modules.
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);

  for (i = 2; i < argc; i++)
    {
      snprintf (query, 4095, "SELECT initial_tId, bId, lane FROM reaction WHERE rId = %s", argv [i]);
      if (mysql_query (con, query))
	finish_with_error (con);

      result = mysql_store_result (con);
      if (!result)
	continue;

      row = mysql_fetch_row (result);
      if (!row)
	continue;
      b.id = strtoull (row [1], NULL, 0);
      lane = atoi (row [2]);
      snprintf (query, 4095, "SELECT rle, period, combined_width, combined_height, offX, offY FROM target WHERE tId = %s", row [0]);
      mysql_free_result (result);

      if (mysql_query (con, query))
	finish_with_error (con);

      result = mysql_store_result (con);
      if (!result)
	continue;

      row = mysql_fetch_row (result);
      if (!row)
	continue;
      pat_from_string (row [0]);
      t.pat = pat_compact (&lab [0], NULL);
      t.nph = atoi (row [1]);
      width = atoi (row [2]);
      height = atoi (row [3]);
      offX = atoi (row [4]);
      offY = atoi (row [5]);
      mysql_free_result (result);
      t.X = lab [0].left;
      t.Y = lab [0].top;
      t.top = t.Y - offY;
      t.bottom = t.top + height - 1;
      t.left = t.X - offX;
      t.right = t.left + width - 1;

      snprintf (query, 4095, "SELECT b.oId, rle, dx, dy, dt, base_x, base_y, reference, lane_dx, lane_dy, lanes_per_height, lanes_per_width, extra_lanes "
				    "FROM bullets b LEFT JOIN objects USING (oId) WHERE bId = %llu", b.id);

      if (mysql_query (con, query))
	finish_with_error (con);

      result = mysql_store_result (con);
      if (!result)
	continue;

      row = mysql_fetch_row (result);
      if (!row)
	continue;
      b.oId = strtoull (row [0], NULL, 0);
      pat_from_string (row [1]);
      b.p = pat_compact (&lab [0], NULL);
      b.dx = atoi (row [2]);
      b.dy = atoi (row [3]);
      b.dt = atoi (row [4]);
      b.base_x = atoi (row [5]);
      b.base_y = atoi (row [6]);
      if (strcmp (row [7], "TOPLEFT") == 0)
	b.reference = topleft;
      else if (strcmp (row [7], "TOPRIGHT") == 0)
	b.reference = topright;
      else if (strcmp (row [7], "BOTTOMLEFT") == 0)
	b.reference = bottomleft;
      else if (strcmp (row [7], "BOTTOMRIGHT") == 0)
	b.reference = bottomright;
      b.lane_dx = atoi (row [8]);
      b.lane_dy = atoi (row [9]);
      b.lanes_per_height = atoi (row [10]);
      b.lanes_per_width = atoi (row [11]);
      b.extra_lanes = atoi (row [12]);
      mysql_free_result (result);

      tgt_collide (&t, &b, lane, &dummy, &dummy, &dummy);
      printf ("#P %d 0\n#C reaction: %s\n", (i-2)*200, argv [i]);
      pat_dump (&lab [0], false);
      printf ("\n");
    }
}

