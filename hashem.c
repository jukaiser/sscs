// Test the hash function.
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "database.h"


main (int argc, char **argv)

{
  reaction fake;
  target my_tgt;
  ROWID i, n;

  config_load (argv [1]);
  db_init ();
  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN, MAX_FIND);

  n = strtoull (argv [2], NULL, 0);

  for (i = 1; i <= n; i++)
    {
      fake.tId = i;
      db_target_fetch (&fake, &my_tgt);
      printf ("Hash: %u\n", pat_GSF_hash (my_tgt.pat));

      pat_deallocate (my_tgt.pat);
      free (my_tgt.pat);
    }
}
