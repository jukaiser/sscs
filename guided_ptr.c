/* Wrapper for void * to keep track of # references so we know when free () would be OK */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "guided_ptr.h"


guided *guide_alloc (void *p)
// wrap p into an fresh guided *

{
  guided *gp = malloc (sizeof (guided));
  if (!gp)
    {
      perror ("guide - malloc");
      exit (2);
    }

  gp->ref_count = 0;
  gp->ptr = p;

  return gp;
}


void guide_link (guided *gp)
// Count yet another link to our pointer

{
  gp->ref_count++;
}


bool guide_unlink (guided *gp)
// decrease link count. Return false if gp->ptr and gp should be free()d

{
  gp->ref_count--;
  return (gp->ref_count > 0);
}


void guide_dealloc (guided *gp)
// deallocate () our object. Assume that gp->ptr has been already free()d

{
  assert (gp->ref_count <= 0);

  gp->ptr = NULL;
  free (gp);
}
