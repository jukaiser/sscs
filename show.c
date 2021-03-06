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


main (int argc, char **argv)

{
  char query [4096];
  bullet b;
  int phase, lane, nph;
  target t;
  int i, delay, dummy, offX, offY, width, height;
  MYSQL_RES *result;
  MYSQL_ROW row;

  config_load (argv [1]);

  db_init ();

  // Initialize all modules.
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);

  for (i = 2; i < argc; i++)
    {
      snprintf (query, 4095, "SELECT initial_tId, initial_phase, bId, lane FROM reaction WHERE rId = %s", argv [i]);
      if (mysql_query (con, query))
	finish_with_error (con);

      result = mysql_store_result (con);
      if (!result)
	continue;

      row = mysql_fetch_row (result);
      if (!row)
	continue;
      phase = atoi (row [1]);
      b.id = strtoull (row [2], NULL, 0);
      lane = atoi (row [3]);
      db_target_reload (&t, strtoull (row [0], NULL, 0), phase);

      snprintf (query, 4095, "SELECT b.oId, rle, dx, dy, dt, base_x, base_y, reference, lane_dx, lane_dy, lanes_per_height, lanes_per_width, extra_lanes "
				    "FROM bullet b LEFT JOIN object USING (oId) WHERE bId = %llu", b.id);

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

      tgt_collide (&t, &b, lane, &delay, &dummy, &dummy, &dummy);
// fprintf (stderr, "PRE GAP: %d\n", delay);
      printf ("#P %d 0\n#C reaction: %s\n", (i-2)*200, argv [i]);
      pat_dump (&lab [0], false);
      printf ("\n");
    }
}

