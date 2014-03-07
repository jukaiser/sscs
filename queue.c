/* Maintaining a weighted queue */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include "config.h"
#include "pattern.h"
#include "reaction.h"
#include "queue.h"


// one file will be about one MB in size ...
// Just for fun. Performance impact of chosing a non perfect size is not too severe
#define CHUNK	65534


typedef struct
  {
    reaction r [CHUNK];
    int	     cost;
    int	     n_get;
    int      n_put;
    int	     n_retrieve;
    int	     n_store;
  } chunk;

static chunk *chunks = NULL;
static int current = 0;
static int max = -1;


static void purge_to_disk (int cost)

{
  char fname [4096];
  FILE *data;
  chunk *cp;
  int  to_save, slot;

  slot = cost % (MAXDELTA+1);
  cp = &chunks [slot];
  assert (cost == cp->cost);

  to_save = cp->n_put - cp->n_get;
  assert (to_save > 0 && to_save <= CHUNK);

  snprintf (fname, 4095, F_TEMPFILES, slot, cost, cp->n_store++);
  data = fopen (fname, "wb");

  printf ("purge_to_disk (%d): saving %d entries to '%s'\n", cost, to_save, fname);
  if (!data ||
      fwrite (&cp->cost, sizeof (int), 1, data) != 1 ||
      fwrite (&to_save, sizeof (int), 1, data) != 1 ||
      fwrite (&cp->r [cp->n_get], sizeof (reaction), to_save, data) != to_save)
    {
      fprintf (stderr, "Error writing to '%s': ", fname);
      perror ("purge_to_disk ()");
      exit (2);
    }

  fclose (data);

  // chunk is unused now ...
  cp->n_get = 0;
  cp->n_put = 0;
}


static void read_from_disk (int cost)

{
  char fname [4096];
  FILE *data;
  chunk *cp;
  int  to_load, slot;

  slot = cost % (MAXDELTA+1);
  cp = &chunks [slot];

  assert (cp->n_get == cp->n_put);

  snprintf (fname, 4095, F_TEMPFILES, slot, cost, cp->n_retrieve++);
  data = fopen (fname, "rb");

  if (!data ||
      fread (&cp->cost, sizeof (int), 1, data) != 1 ||
      cp->cost != cost ||
      fread (&to_load, sizeof (int), 1, data) != 1 ||
      to_load <= 0 || to_load > CHUNK ||
      fread (cp->r, sizeof (reaction), to_load, data) != to_load)
    {
      fprintf (stderr, "Error reading from '%s': ", fname);
      perror ("read_from_disk ()");
      exit (2);
    }

  fclose (data);
  printf ("read_from_disk (%d): read back %d entries from '%s'\n", cost, to_load, fname);
  unlink (fname);

  // We should have some data for our customer now
  cp->n_get = 0;
  cp->n_put = to_load;
}


void queue_init (void)
/* (Re)initialise our q */

{
  int slot;

  // Do we have some old qs left to discard?
  if (chunks)
    free (chunks);

  chunks = calloc (sizeof (chunk), MAXDELTA+1);
  if (!chunks)
    {
      perror ("queue_init () - calloc()");
      exit (2);
    }
  current = 0;

  // Just to be safe.
  for (slot = 0; slot <= MAXDELTA; slot++)
    {
      chunks [slot].n_get = 0;
      chunks [slot].n_put = 0;
      chunks [slot].n_retrieve = 0;
      chunks [slot].n_store = 0;
    }
}


bool queue_insert (int cost, ROWID tId, uint8_t  b, uint8_t  lane, uint8_t  delta)

{
  chunk *cp;
  reaction *r;
  int slot;

  assert (chunks);
  assert (cost == 0 || (cost > current && cost <= current + MAXDELTA));

  if (cost > max) max = cost;

  slot = cost % (MAXDELTA+1);
  cp = &chunks [slot];

  // If this is the first item in the current slot we have to do some bookkeeping.
  if (cp->n_put == cp->n_get)
    {
      cp->cost = cost;
      cp->n_get = 0;
      cp->n_put = 0;
      cp->n_retrieve = 0;
      cp->n_store = 0;
    }

  assert (cp->cost == cost);

  // do we have enough space left?
  if (cp->n_put >= CHUNK)
    purge_to_disk (cost);

  // Insert new reaction at the end of the selected chunk
  r = &cp->r [cp->n_put++];
  r->rId   = 0;
  r->tId   = tId;
  r->cost  = cost;
  r->delta = delta;
  r->b     = b;
  r->lane  = lane;

  // TO DO: our current implementation either fails fatally or is successful. So returning true for success is not really indicated.
  return true;
}


reaction *queue_grabfront (void)

{
  chunk *cp;
  int slot;

  assert (chunks);

  while (current <= max)
    {
      slot = current % (MAXDELTA+1);
      cp = &chunks [slot];

      assert (cp->n_get >= 0);
      assert (cp->cost == current);

      // is there at least one entry left in the current chunk?
      if (cp->n_get < cp->n_put)
	break;

      // Do we have a(nother) saved chunk on disk?
      if (cp->n_retrieve < cp->n_store)
	{
	  read_from_disk (current);
	  break;
	}
 
      // no more reactions of the current cost found.
      current++;
    }

  // No more work left!
  if (current > MAXCOST || current > max) return NULL;

  // we have at least one item left!
  assert (cp->n_get < cp->n_put);

  return &cp->r [cp->n_get++];
}


void queue_info (void)

{
  printf ("Q: current=%d, max=%d\n", current, max);
}


void queue_save_state (const char *fname)

{
  FILE *data = fopen (fname, "wb");

  if (!data ||
      fwrite (&current, sizeof (int), 1, data) != 1 ||
      fwrite (&max, sizeof (int), 1, data) != 1 ||
      fwrite (chunks, sizeof (chunk), MAXDELTA+1, data) != MAXDELTA+1)
    {
      fprintf (stderr, "Error writing to '%s': ", fname);
      perror ("queue_save_state ()");
      exit (2);
    }

  fclose (data);
}


void queue_restore_state (const char *fname)

{
  assert (chunks);

  FILE *data = fopen (fname, "rb");

  if (!data ||
      fread (&current, sizeof (int), 1, data) != 1 ||
      fread (&max, sizeof (int), 1, data) != 1 ||
      fread (chunks, sizeof (chunk), MAXDELTA+1, data) != MAXDELTA+1)
    {
      fprintf (stderr, "Error reading from '%s': ", fname);
      perror ("queue_restore_state ()");
      exit (2);
    }

  fclose (data);
}


void queue_extend_depth (int delta)

{
  // If we are extending an restore search we have to go deeper the the last time.
  if (current > MAXCOST)
    MAXCOST = current + delta - 1;
  else
    MAXCOST += delta;
}
