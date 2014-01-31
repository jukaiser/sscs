/* demo for DB storage of patterns, like */


#include <stdio.h>
#include <stdlib.h>


#include "config.h"
#include "pattern.h"
#include "database.h"


extern pattern *lab;


main ()

{
  int parts, i, j;
  FILE *f;

  db_init ();
  srand (13045759);

  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN);

  f = fopen ("test/start-targets.pat", "r");
  parts = 1;
  while (!feof (f))
    pat_load (&lab [parts++], f);
  fclose (f);

  for (i = 0; i < 10000000; i++)
    {
      pat_copy (&lab [0], (rand ()%20)-(rand ()%20), (rand ()%20)-(rand ()%20), &lab [1+(rand()%(parts-1))]);
      for (j = 0; j < 3; j++)
	pat_add (&lab [0], (rand ()%20)-(rand ()%20), (rand ()%20)-(rand ()%20), &lab [1+(rand()%(parts-1))]);

      if (db_target_exists (pat_rle (&lab [0]), W(&lab[0]), H(&lab [0])))
	printf ("Treffer %s\n", pat_rle (&lab [0]));
      // else
        // db_target_insert (pat_rle (&lab [0]), W(&lab[0]), H(&lab [0]));
    }

  char *xx =  "11bo$10bobo$9bo2bo$10b2o8$6b2o$2bo2bobo$bobo2bo$o2bo$b2o$15bo$14bobo$14bobo$15bo!";
  if (db_target_exists (xx, 0, 0))
    printf ("Gegenprobe: %s\n", xx);
}
