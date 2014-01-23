/* Maintaining a weighted queue */

#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "queue.h"


typedef struct _entry
  {
    void          *value;
    struct _entry *next;
  } entry;

static void *qs [MAXCOST] = {NULL};
static int current = 0;
static int max = -1;


void dump_qs ()

{
  int i;

  printf ("\nQ-Dump [%d, %d]\n", current, max);
return;
  for (i = current; i <= max; i++)
    {
      printf ("q [%d]: ", i);
      entry *e = qs [i];
      if (!e)
	printf (" is empty");
      else while (e)
	{
	  printf ("%p->(%p,%p) ", e, e->value, e->next);
	  e = e->next;
	}
      printf ("\n");
    }
}


void queue_init (void)
/* (Re)initialise our q */

{
  int i;

  for (i = 0; i <= max; i++)
    {
      if (qs [i])
	{
	  /* TO DO: we should probably free () any entries in qs [i] and their values ... */
	  free (qs [i]);
	  qs [i] = NULL;
	}
    }
  current = 0;
  max = -1;
}


bool queue_insert (int cost, void *val)
{
  // Sanity 4 all!
  if (cost < current)
    return false;
  if (cost > max)
    {
      // We don't have enough qs []!
      if (cost >= MAXCOST)
	return false;
      max = cost;
    }

  entry *e = (entry *) malloc (sizeof (entry));
  if (!e) return false;

  // Insert new entry at front of q
  // Note: this results in a kind of "depth first" search. Salvos of several cheap bullets will be considered first.
  e->value = val;
  e->next = qs [cost];
  qs [cost] = e;

  return true;
}


void *queue_grabfront (void)

{
// printf ("GRAB!\n"); fflush (stdout);
// dump_qs (); fflush (stdout);
  // find first non empty q
  while (current <= max && !qs [current])
     {
     current++;
// printf ("current = %d|%p\n", current, qs [current]); fflush (stdout);
// dump_qs (); fflush (stdout);
     }

  // complete q empty?
  if (current > max) return NULL;
// dump_qs (); fflush (stdout);

  entry *e = qs [current];
  void *v = e->value;
  qs [current] = e->next;
  free (e);

  return v;
}
