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

  f = fopen ("test/pi.pat", "r");
  pat_load (target, f);
  fclose (f);

  f = fopen ("test/glider.pat", "r");
  pat_load (t, f);
  fclose (f);
  pat_copy (bullet, -t->left, -t->top, t);

  for (i = 0; i < 31; i++)
    {
      pat_copy (&lab [0], 0, 0, target);
      pat_add  (&lab [0], lab [0].left-4, lab [0].bottom+1-i, bullet);
      pat_dump (&lab [0]);
    }
}

