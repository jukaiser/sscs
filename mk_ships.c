/* ship generator (fills DB Table space_ships */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "pattern.h"


pattern *t, *temp;


typedef struct
  {
    char *name;
    char *pat;
    int  dx, dy, dt;
  } ship;


#define F_FMT "../gencols/obj/%s.life"

char *ships [] =
  {
    "glider_ne", "glider_nw", "glider_se", "glider_sw",
    "hwss_e", "hwss_n", "hwss_s", "hwss_w",
    "lwss_e", "lwss_n", "lwss_s", "lwss_w",
    "mwss_e", "mwss_n", "mwss_s", "mwss_w"
  };

main (int argc, char **argv)

{
  char fname [1024], *rle;
  FILE *f;
  int i, j;

  lab_allocate (20, 20, 5, 2);

  for (i = 0; i < (sizeof (ships) / sizeof (char*)); i++)
    {
      sprintf (fname, F_FMT, ships [i]);
      f = fopen (fname, "r");
if (!f) { perror (fname); exit (2); }

      lab_init ();
      pat_load (f);

      for (j = 0; j < 4; j++)
	pat_generate (&lab [j], &lab [j+1]);

      for (j = 0; j < 4; j++)
	{
	  rle = pat_rle (&lab [j]);
	  printf ("INSERT INTO objects VALUES (NULL, '%s', '%s', %d, %d, %d, %d, 4, %d, %d, %d);\n",
		  rle, ships [i], W(&lab[j]), H(&lab[j]), lab[4].left-lab[0].left, lab[4].top-lab[0].top, j, lab[j].left-lab[0].left, lab[j].top-lab[0].top);
	  if (strcmp (rle, "o$b2o$2o!") == 0)
	    printf ("INSERT INTO bullets VALUES (1,'gl-se',LAST_INSERT_ID(),-4,0,'BOTTOMLEFT',0,-1,1,1,8);\n");
	}
    }
}


