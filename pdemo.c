/* dummy test thingy for pattern handling*/

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "pattern.h"


#define W(p)    ((p)->right-(p)->left+1)
#define H(p)    ((p)->bottom-(p)->top+1)

extern pattern *lab;

pattern *target, *t, *bullet;

main ()

{
  int i;
  FILE *f;

  lab_allocate (MAXWIDTH, MAXHEIGHT, MAXGEN);
  target = pat_allocate (NULL, MAXWIDTH, MAXHEIGHT);
  t = pat_allocate (NULL, MAXWIDTH, MAXHEIGHT);
  bullet = pat_allocate (NULL, MAXWIDTH, MAXHEIGHT);

  f = fopen ("test/glider.pat", "r");
  pat_load (t, f);
  fclose (f);
  pat_copy (bullet, -t->left, -t->top, t);

  // f = fopen ("test/start-targets.pat", "r");
  f = fopen ("test/block.pat", "r");

  while (!feof (f))
    {
      pat_load (target, f);

      for (i = 0; i < 14; i++)
	{
	  pat_copy (&lab [0], 0, 0, target);
	  pat_add  (&lab [0], lab [0].left-4, lab [0].bottom+1-i, bullet);
	  // printf ("x = %d, y = %d, rule = B3/S23\n%s\n\n", W(&lab[0]), H(&lab [0]), pat_rle (&lab [0]));
pat_dump (&lab [0]);
	}
    }

  fclose (f);
}

