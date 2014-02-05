/* ship generator (fills DB Table space_ships */

#include <stdlib.h>
#include <stdio.h>

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
  char fname [1024];
  FILE *f;
  int i, j;

  lab_allocate (20, 20, 5);

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
        printf ("INSERT INTO space_ships VALUES (NULL, '%s', '%s:%d', %d, %d, %d, %d, 4);\n", pat_rle (&lab [j]), ships [i], j, W(&lab[j]), H(&lab[j]), lab[4].top-lab[0].top, lab[4].left-lab[0].left);
    }
}


