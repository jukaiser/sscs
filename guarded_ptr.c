/* Wrapper for void * to keep track of # references so we know when free () would be OK */


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "guarded_ptr.h"


guarded *guard_alloc (void *p)
// wrap p into an fresh guarded *

{
  guarded *gp = malloc (sizeof (guarded));
  if (!gp)
    {
      perror ("guarded - malloc");
      exit (2);
    }

  gp->ref_count = 0;
  gp->ptr = p;

  return gp;
}


void guard_link (guarded *gp)
// Count yet another link to our pointer

{
  gp->ref_count++;
}


bool guard_unlink (guarded *gp)
// decrease link count. Return false if gp->ptr and gp should be free()d

{
  gp->ref_count--;
  return (gp->ref_count > 0);
}


void guard_dealloc (guarded *gp)
// deallocate () our object. Assume that gp->ptr has been already free()d

{
  assert (gp->ref_count <= 0);

  gp->ptr = NULL;
  free (gp);
}
