#include <stdio.h>
#include <stdlib.h>


#include "config.h"


main (int argc, char**argv)

{
  int i;
  
  if (argc != 2) exit (1);

  config_load (argv [1]);

  
  printf ("MAXPERIOD=%d\n", MAXPERIOD);
  printf ("DBNAME='%s'\n", DBNAME);
  printf ("RELATIVE=%d\n", RELATIVE);

  printf ("COSTS=");
  for (i = 0; i < nCOSTS; i++)
    printf ("%d ", COSTS [i]);
  printf ("\n");
}
