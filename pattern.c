/* Dealing with GoL patterns. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include <assert.h>

#include "config.h"
#include "pattern.h"


#define ALLOC(t,arr,n)  {arr = (t *)calloc (n, sizeof (t)); if (!arr) {fprintf (stderr, "calloc () failed!\n"); exit (2);}}
//#define ALLOC(t,arr,n)        {arr = (t *)calloc (n, sizeof (t)); if (!arr) {fprintf (stderr, "calloc () failed!\n"); exit (2);} else printf ("%p -> %ld\n", arr, n * sizeof (t));}

#define W(p)    ((p)->right-(p)->left+1)
#define H(p)    ((p)->bottom-(p)->top+1)


pattern *lab = NULL;
int maxX, maxY, maxGen;

static pattern *temp = NULL;
static char rle [MAX_RLE];


void pat_init (pattern *p)
// CAVE: we assume that every other function just care for cells within the bounding box (be carefull, when you extend that box!)

{
  assert (p);
  assert (p->cell);
  assert (p->cell [0]);
  assert (p->sizeY > 0);
  assert (p->sizeX > 0);

  // memset (p->cell [0], DEAD, p->sizeY * p->sizeX);
  p->top    = 0;
  p->bottom = -1;
  p->left   = 0;
  p->right  = -1;
}


pattern *pat_allocate (pattern *p, int sx, int sy)

{
  pattern *ret;
  int y;

  assert (sy > 0);
  assert (sx > 0);

  if (p)
    ret = p;
  else
    ALLOC(pattern,ret,1);

  ret->sizeX  = sx;
  ret->sizeY  = sy;

  ALLOC (char *,ret->cell,sy);
  ALLOC (char,ret->cell [0],sy*sx);
  for (y = 1; y < sy; y++)
    ret->cell [y] = ret->cell [0] + y*sx;
  assert (ret->cell [sy-1] + sx == ret->cell [0] + sx*sy);

  pat_init (ret);

  return ret;
}


void pat_deallocate (pattern *pat)

{
  assert (pat->sizeY);
  assert (pat->sizeX);
  assert (pat->cell);
  assert (pat->cell [0]);

  free (pat->cell [0]);
  free (pat->cell);
  pat->cell   = NULL;
  pat->sizeX  = 0;
  pat->sizeY  = 0;
  pat->top    = 0;
  pat->bottom = -1;
  pat->left   = 0;
  pat->right  = -1;
}


void lab_allocate (int _maxX, int _maxY, int _maxGen)

{
  int g;

  assert (_maxX > 0);
  assert (_maxY > 0);
  assert (_maxGen > 0);

  maxX = _maxX;
  maxY = _maxY;
  maxGen = _maxGen;

  ALLOC(pattern,lab,maxGen);

  for (g = 0; g < maxGen; g++)
    pat_allocate (&lab [g], maxX, maxY);

  assert (lab [0].sizeX == maxX && lab [0].sizeY == maxY);
  assert (lab [0].sizeX > 0 && lab [0].sizeY > 0);

  temp = pat_allocate (NULL, maxX, maxY);
}


void lab_init ()

{
  int g;

  assert (lab);

  for (g = 0; g < maxGen; g++)
    pat_init (&lab [g]);

  assert (lab [0].sizeX > 0 && lab [0].sizeY > 0);
}


void pat_shrink_bbox (pattern *p)

{
  int x, y;

  if (H(p) <= 0) return;

  int  ymin = p->top, ymax = p->bottom;
  int  xmin = p->left, xmax = p->right;

  // classical min/max search.
  p->top = p->sizeY-1; p->bottom = 0;
  p->left = p->sizeX-1; p->right = 0;
  for (y = ymin; y <= ymax; y++)
    for (x = xmin; x <= xmax; x++)
      if (p->cell [y][x] == ALIVE)
        {
          if (p->top > y) p->top = y;
          if (p->bottom < y) p->bottom = y;
          if (p->left > x) p->left = x;
          if (p->right < x) p->right = x;
        }

  if (H(p) <= 0)
    return;

  // if we did NOT shrink one edge of the bbox, we should make sure there is empty space around it.
  int y1 = (p->top > 0) ? p->top-1 : 0;
  int y2 = (p->bottom < p->sizeY-1) ? p->bottom+1 : p->sizeY-1;
  int x1 = (p->left > 0) ? p->left-1 : 0;
  int x2 = (p->right < p->sizeX-1) ? p->right+1 : p->sizeX-1;
  if (p->top == ymin && ymin > 0)
    for (x = x1; x <= x2; x++)
      p->cell [ymin-1][x] = DEAD;
  if (p->bottom == ymax && ymax < p->sizeY-1)
    for (x = x1; x <= x2; x++)
      p->cell [ymax+1][x] = DEAD;
  if (p->left == xmin && xmin > 0)
    for (y = y1; y <= y2; y++)
      p->cell [y][xmin-1] = DEAD;
  if (p->right == xmax && xmax < p->sizeX-1)
    for (y = y1; y <= y2; y++)
      p->cell [y][xmax+1] = DEAD;
}



bool pat_generate (pattern *p1, pattern *p2)
// TO DO: support for other Rules then B2/S32, support for lifeHistory look alikes

{
  int neighbors [p2->sizeY][p2->sizeX];
  int x, y;

  // If p1 contains no living cells, then p2 won't either!
  if (H(p1) <= 0)
    {
      p2->top = 0;
      p2->bottom = -1;
      p2->left = 0;
      p2->right = -1;
      return 0;
    }

  assert (p2->sizeY >= p1->sizeY);
  assert (p2->sizeX >= p1->sizeX);

  // We can not grow our bounding box faster then one cell per turn ...
  // And we have to stay off the border!
  p2->top    = (p1->top > 0)              ? p1->top-1    : 0;
  p2->bottom = (p1->bottom < p1->sizeY-1) ? p1->bottom+1 : p1->sizeY-1;
  p2->left   = (p1->left > 0)             ? p1->left-1   : 0;
  p2->right  = (p1->right < p1->sizeX-1)  ? p1->right+1  : p1->sizeX-1;

  // Note: even if we are looking at a single pixel in one of the corners ...
  // as long as sizeX and sizeY are > 1, we should end up with at least a 2x2 box.
  // We NEED this size or we will be adressing stuff beyond p1->cell [][]
  assert (p2->top < p2->bottom);
  assert (p2->left < p2->right);
  assert (p2->top >= 0);
  assert (p2->bottom < p2->sizeY);
  assert (p2->left >= 0);
  assert (p2->right < p2->sizeX);

  // first the core of the pattern ... every cell has 8 neighbors
  for (y = p2->top+1; y <= p2->bottom-1;  y++)
    for (x = p2->left+1; x <= p2->right-1;  x++)
      {
	neighbors [y][x] = (p1->cell [y-1][x-1] == ALIVE) + (p1->cell [y-1][x  ] == ALIVE) + (p1->cell [y-1][x+1] == ALIVE) +
			   (p1->cell [y  ][x-1] == ALIVE) +                                  (p1->cell [y  ][x+1] == ALIVE) +
			   (p1->cell [y+1][x-1] == ALIVE) + (p1->cell [y+1][x  ] == ALIVE) + (p1->cell [y+1][x+1] == ALIVE);
      }

  // The outer cells don't have valid neighbors on every side ... handle them!
  // first the corners
  neighbors [p2->top][p2->left]     =                                                   (p1->cell [p2->top  ][p2->left+1]     == ALIVE) +
				      (p1->cell [p2->top+1][p2->left  ]     == ALIVE) + (p1->cell [p2->top+1][p2->left+1]     == ALIVE);

  neighbors [p2->top][p2->right]    = (p1->cell [p2->top  ][p2->right-1]    == ALIVE)                                                   +
				      (p1->cell [p2->top+1][p2->right-1]    == ALIVE) + (p1->cell [p2->top+1][p2->right  ]    == ALIVE);

  neighbors [p2->bottom][p2->left]  = (p1->cell [p2->bottom-1][p2->left  ]  == ALIVE) + (p1->cell [p2->bottom-1][p2->left+1]  == ALIVE) +
				                                                        (p1->cell [p2->bottom  ][p2->left+1]  == ALIVE);

  neighbors [p2->bottom][p2->right] = (p1->cell [p2->bottom-1][p2->right-1] == ALIVE) + (p1->cell [p2->bottom-1][p2->right  ] == ALIVE) +
				      (p1->cell [p2->bottom  ][p2->right-1] == ALIVE);

  // now the edges.
  for (x = p2->left+1; x <= p2->right-1;  x++)
    {
      neighbors [p2->top][x] = (p1->cell [p2->top  ][x-1] == ALIVE) +                                        (p1->cell [p2->top  ][x+1] == ALIVE) +
			       (p1->cell [p2->top+1][x-1] == ALIVE) + (p1->cell [p2->top+1][x  ] == ALIVE) + (p1->cell [p2->top+1][x+1] == ALIVE);
    }
  for (x = p2->left+1; x <= p2->right-1;  x++)
    {
      neighbors [p2->bottom][x] = (p1->cell [p2->bottom-1][x-1] == ALIVE) + (p1->cell [p2->bottom-1][x  ] == ALIVE) + (p1->cell [p2->bottom-1][x+1] == ALIVE) +
			          (p1->cell [p2->bottom  ][x-1] == ALIVE) +                                           (p1->cell [p2->bottom  ][x+1] == ALIVE);
    }
  for (y = p2->top+1; y <= p2->bottom-1;  y++)
    {
      neighbors [y][p2->left] = (p1->cell [y-1][p2->left  ] == ALIVE) + (p1->cell [y-1][p2->left+1] == ALIVE) +
			                                                (p1->cell [y  ][p2->left+1] == ALIVE) +
			        (p1->cell [y+1][p2->left  ] == ALIVE) + (p1->cell [y+1][p2->left+1] == ALIVE);
    }
  for (y = p2->top+1; y <= p2->bottom-1;  y++)
    {
      neighbors [y][p2->right] = (p1->cell [y-1][p2->right-1] == ALIVE) + (p1->cell [y-1][p2->right  ] == ALIVE) +
			         (p1->cell [y  ][p2->right-1] == ALIVE) +                                        +
			         (p1->cell [y+1][p2->right-1] == ALIVE) + (p1->cell [y+1][p2->right  ] == ALIVE);
    }

  for (y = p2->top; y <= p2->bottom;  y++)
    for (x = p2->left; x <= p2->right;  x++)
      p2->cell [y][x] = ((neighbors [y][x] == 3) || (neighbors [y][x] == 2 && p1->cell [y][x] == ALIVE)) ? ALIVE : DEAD; // THIS is B2/S23!!

  // update bounding box
  pat_shrink_bbox (p2);

  // "Todos muertos"?
  return (H(p2) > 0);
}


void pat_add (pattern *p1, int offX, int offY, pattern *p2)

{
  int x, y, nTop, nBottom, nLeft, nRight;

  // Easy case: target pattern is empty ...
  if (W(p1) <= 0)
    {
      pat_copy (p1, offX, offY, p2);
      return;
    }

printf ("%d, %d, %d\n", offY, p2->bottom, p1->sizeY); fflush (stdout);
  assert (offY + p2->bottom < p1->sizeY);
  assert (offX + p2->right < p1->sizeX);

  // adjust bounding box
  nTop = p1->top; if (p1->top > p2->top+offY) nTop = p2->top+offY;
  nBottom = p1->bottom; if (p1->bottom < p2->bottom+offY) nBottom = p2->bottom+offY;
  nLeft = p1->left; if (p1->left > p2->left+offX) nLeft = p2->left+offX;
  nRight = p1->right; if (p1->right < p2->right+offX) nRight = p2->right+offX;

  // Fill the gap between old and new bbox with DEAD cells.
  for (y = nTop; y < p1->top; y++)
    for (x = nLeft; x <= nRight; x++)
      p1->cell [y][x] = DEAD;
  for (y = p1->bottom+1; y <= nBottom; y++)
    for (x = nLeft; x <= nRight; x++)
      p1->cell [y][x] = DEAD;
  for (y = nTop; y <= nBottom; y++)
    for (x = nLeft; x < p1->left; x++)
      p1->cell [y][x] = DEAD;
  for (y = nTop; y <= nBottom; y++)
    for (x = p1->right+1; x < nRight; x++)
      p1->cell [y][x] = DEAD;
  p1->top = nTop;
  p1->bottom = nBottom;
  p1->left = nLeft;
  p1->right = nRight;

  // Now we can copy the pattern over ...
  for (y = p2->top; y <= p2->bottom;  y++)
    for (x = p2->left; x <= p2->right;  x++)
      if (p2->cell [y][x] == ALIVE)
	p1->cell [y+offY][x+offX] = ALIVE;
}


void pat_copy (pattern *p1, int offX, int offY, pattern *p2)
// Variant of pat_add: discard old content first.

{
  int x, y;

  assert (offY + p2->bottom < p1->sizeY);
  assert (offX + p2->right < p1->sizeX);

  // Copy ALL cells! even dead ones.
  for (y = p2->top; y <= p2->bottom;  y++)
    for (x = p2->left; x <= p2->right;  x++)
      p1->cell [y+offY][x+offX] = p2->cell [y][x];

  // Transfer the bounding box.
  p1->top    = p2->top + offY;
  p1->bottom = p2->bottom + offY;
  p1->left   = p2->left + offX;
  p1->right  = p2->right + offX;
}


void pat_remove (pattern *p1, int offX, int offY, pattern *p2)

{
  int x, y;

  assert (offY + p2->bottom < p1->sizeY);
  assert (offX + p2->right < p1->sizeX);

  for (y = p2->top; y <= p2->bottom;  y++)
    for (x = p2->left; x <= p2->right;  x++)
      if (p2->cell [y][x] != UNDEF)
	p1->cell [y+offY][x+offX] = DEAD;

  // bounding box could only shrink here ...
  pat_shrink_bbox (p1);
}


/*
void pat_envelope (pattern *pat)

{
  int x, y;

  // Sanity check - pat must be valid and have enough space left.
  assert (pat);
  assert (pat->cell);
  assert (pat->cell [0]);
  assert (pat->top > 0);
  assert (pat->bottom < pat->sizeY-1);
  assert (pat->left > 0);
  assert (pat->right < pat->sizeX-1);

  // set any not-ALIVE-cell to UNDEF
  for (y = pat->top; y <= pat->bottom;  y++)
    for (x = pat->left; x <= pat->right;  x++)
      if (pat->cell [y][x] != ALIVE)
	pat->cell [y][x] = UNDEF;
  for (y = pat->top; y <= pat->bottom;  y++)
    {
      pat->cell [y][pat->left-1] = UNDEF;
      pat->cell [y][pat->right+1] = UNDEF;
    }
  for (x = pat->left-1; x <= pat->right+1;  x++)
    {
      pat->cell [pat->top-1][x] = UNDEF;
      pat->cell [pat->bottom+1][x] = UNDEF;
    }

  // Any cell next to a living cell *does* matter.
  for (y = pat->top; y <= pat->bottom;  y++)
    for (x = pat->left; x <= pat->right;  x++)
      if (pat->cell [y][x] == ALIVE)
	{
	  if (pat->cell [y-1][x-1] == UNDEF) pat->cell [y-1][x-1] = DEAD;
	  if (pat->cell [y-1][x  ] == UNDEF) pat->cell [y-1][x  ] = DEAD;
	  if (pat->cell [y-1][x+1] == UNDEF) pat->cell [y-1][x+1] = DEAD;

	  if (pat->cell [y  ][x-1] == UNDEF) pat->cell [y  ][x-1] = DEAD;
	  if (pat->cell [y  ][x+1] == UNDEF) pat->cell [y  ][x+1] = DEAD;

	  if (pat->cell [y+1][x-1] == UNDEF) pat->cell [y+1][x-1] = DEAD;
	  if (pat->cell [y+1][x  ] == UNDEF) pat->cell [y+1][x  ] = DEAD;
	  if (pat->cell [y+1][x+1] == UNDEF) pat->cell [y+1][x+1] = DEAD;
	}

  // update the bounding box.
  // CAVE: pat_shrink_bbox () will undo this!
  pat->top--;
  pat->bottom++;
  pat->left--;
  pat->right++;
}
*/


/*
pattern *pat_compact (pattern *p1, pattern *p2)
// Allocate a copy of p2 which is *just* big enough for the bounding box.

{
  pattern *ret;
  int x, y;

  assert (p1);
  assert (p1->cell);
  assert (p1->cell [0]);
  assert (W(p1)>0 && H(p1)>0);

  ret = pat_allocate (p2, W(p1), H(p1));
  for (y = p1->top; y <= p1->bottom;  y++)
    for (x = p1->left; x <= p1->right;  x++)
      ret->cell [y-p1->top][x-p1->left] = p1->cell [y][x];

  ret->top = 0;
  ret->bottom = H(p1)-1;
  ret->left = 0;
  ret->right = W(p1)-1;

  return ret;
}
*/


void pat_from_string (pattern *pat, const char *str)
// str contains exactly one pattern. load it centered into *pat
// pat must be large enough to hold the pattern.

{
  int  x = 1, y = 1, count = 0, maxX = 1;
  bool rle = false;
  char dCell, lCell, newLine, endPat;
  const char *cp = str;

  assert (pat->sizeX >= temp->sizeX);
  assert (pat->sizeY >= temp->sizeY);

  pat_init (temp);
  memset (temp->cell [0], DEAD, temp->sizeY * temp->sizeX); // we don't know the width of the pattern before we have loaded it ...

  /* Try to find out which format the str is in.
     Supported formats are:
	- '.' means DEAD, '*' means ALIVE, line feeds begin a new line in the pattern. Other WS is *ignored* (similar to standard .life files)
	- rle (b meeans DEAD, o means ALIVE, pattern lines are ended by $, bo$ can be prefix by a repeat count, ! ends pattern) WS is ignored
	- gencols output format. '.' means DEAD, '*' means alive, '!' begins a new pattern line, anything else terminates pattern.

     For convenience we are very tollerant:
	- Lines beginning with # are ignored (Comments)
	- emtpy lines (or lines consisting of WS) at the start of the pattern are ignored
	- lines beginning with something like "x = " are ignored (but indicate rle mode)

     Mode is determined heuristically:
	- Is there a line starting with "x ="? -> RLE mode
	- Look at the first "non garbage" char: is it a number or o, b? -> RLE mode
	- non RLE mode: does pattern contain a '!'? -> gencols mode
  */
  while (*cp)
    {
      if (*cp == '#')
	{
	  while (*cp && *cp !=  '\n')
	    cp++;
	}
      else if (*cp == 'x')
	{
	  rle = true;
	  dCell = 'b';
	  lCell = 'o';
	  newLine = '$';
	  endPat = '!';

	  while (*cp && *cp !=  '\n')
	    cp++;
	}
      else if (!isspace (*cp))
	break;

      if (*cp)
	cp++;
    }

  // Here: we are at the first non garbage char
  if (!rle)
    {
      if (*cp == '.' || *cp == '*')
	{
	  dCell = '.';
	  lCell = '*';
	  if (strchr (cp, '!'))
	    {
	      newLine = '!';
	      endPat = ' ';
	    }
	  else
	    {
	      newLine = '\n';
	      endPat = '\0';
	    }
	}
      else
	{
	  rle = true;
	  dCell = 'b';
	  lCell = 'o';
	  newLine = '$';
	  endPat = '!';
	}
    }

  // now for the pattern itself ...
  while (*cp)
    {
      if (rle && isdigit (*cp))
	count = count*10 + *cp - '0';
      else if (*cp == dCell || *cp == lCell)
	{
	  if (!count)
	    count = 1;
	  if (x + count >= temp->sizeX-1)
	    {
	      fprintf (stderr, "Pattern too wide!\n");
	      exit (1);
	    }
	  for (; count > 0; count--)
	    temp->cell [y][x++] = (*cp == lCell) ? ALIVE : DEAD;
	  if (x > maxX) maxX = x;
	}
      else if (*cp == newLine)
	{
	  y += count ? count : 1;
	  if (y >= temp->sizeY-1)
	    {
	      fprintf (stderr, "Pattern too high!\n");
	      exit (1);
	    }
	  x = 1;
	  count = 0;
	}
      else if (endPat == ' ' || !isspace (*cp))
	break;

      cp++;
    }
  temp->top = 1;
  temp->bottom = y;
  temp->left = 1;
  temp->right = maxX-1;

  // OK. Loading to a temporary space is done. Now we just have to keep this thing.
  // Draw temp centered in lab [0], discarding old content.
  pat_copy  (pat, (pat->sizeX-W(temp))/2, (pat->sizeY-H(temp))/2, temp);
  pat_shrink_bbox (pat);
}


bool is_empty (const char *line)

{
  const char *pos;

  for (pos = line; *pos; pos++)
    if (!isspace (*pos))
      return false;

  return true;
}


void pat_load (pattern *pat, FILE *f)
// f is an open FILE, that contains one or more patterns. Load the one at the current position into *pat and keep file open.
// pat must be large enough to hold the pattern.

{
  char buffer [MAX_RLE];
  char *pos = buffer;
  int  size_left = MAX_RLE;

  /* basic idea: read pattern line by line from f into buffer [] ... then let pat_from_string () do the parsing.
     How to know when a pattern is complete?
     	- skip emtpy or comment lines at the beginning,
	- an ! in an non empty line either means the end of an rle encoded pattern or it mean that line IS an complete gencols-style pattern
	- an empty line after the first non-empty line terminates any normal .life pattern.
  */
  while (!feof (f) && !ferror (f))
    {
      fgets (buffer, MAX_RLE, f);
      if (buffer [0] != '#' && buffer [0] != '\n' && !is_empty (buffer))
	break;
    }

  if (ferror (f))
    {
      perror ("pat_load");
      exit (1);
    }

  pos = buffer + strlen (buffer);
  while (!strchr (pos, '!') && !feof (f) && !ferror (f))
    {
      fgets (pos, size_left, f);
      if (is_empty (pos))
	break;
      size_left -= strlen (pos);
      pos += strlen (pos);
    }

  if (ferror (f))
    {
      perror ("pat_load");
      exit (1);
    }

  pat_from_string (pat, buffer);
}


static void rle_append (char *tgt, int *pos, int nr, char c)
// Helper fun for pat_rle ():
// Append c (optionally preceeded by a number nr) to tgt starting at pos.
// Make sure we do not exceed MAX_RLE!

{
  if (nr <= 0)
    return;

  // Check if we will probably fit ...
  // NOTE: maybe we won't fit if nr is way larger then 10 ... but since we're using snprintf in this case, at least we won't crash
  if (*pos >= MAX_RLE || (nr > 1 && *pos >= MAX_RLE-1))
    {
      fprintf (stderr, "RLE too large - adjust MAX_RLE and recompile!\n");
      exit (2);
    }

  // do we have to output nr?
  if (nr > 1)
    {
      snprintf (tgt+*pos, MAX_RLE-*pos, "%d%c", nr, c);
      *pos += strlen (tgt+*pos);
    }
  else
    {
      // Just write out the char
      tgt [(*pos)++] = c;
    }
}



char *pat_rle (pattern *pat)
// rle encode generation g of our pattern into buf; we may not exceed the given size
// result: 0 terminated rle string - without the header!
/*
  Example: the rle below is a (certain phase of a) ne heading glider

x = 3, y = 3, rule = B3/S23
b2o$obo$2bo!

*/

{
  int x, y, run, pos = 0, nr_lf = 0;
  char current;

  for (y = pat->top; y <= pat->bottom; y++)
    {
      run = 0;
      current = DEAD;

      for (x = pat->left; x <= pat->right; x++)
        {
          if (pat->cell [y][x] == current)
            run++;
          else
            {
              if (nr_lf)
                {
                  rle_append (rle, &pos, nr_lf, '$');
                  nr_lf = 0;
                }
              rle_append (rle, &pos, run, (current == ALIVE) ? 'o' : 'b');
              current = pat->cell [y][x];
              run = 1;
            }
        }

      if (current == ALIVE)
        rle_append (rle, &pos, run, 'o');
      nr_lf++;
    }

  rle_append (rle, &pos, 1, '!');
  rle_append (rle, &pos, 1, '\0');

  return rle;
}


void pat_dump (pattern *p)

{
  if (p)
    {
      printf ("Pattern (%d,%d) [%d,%d,%d,%d]: %p\n", p->sizeY, p->sizeX, p->top, p->bottom, p->left, p->right, p->cell);
      if (p->cell)
	{
	  int x, y;
	  for (y = p->top; y <= p->bottom; y++)
	    {
	      for (x = p->left; x <= p->right; x++)
		putc (p->cell [y][x], stdout);
	      putc ('\n', stdout);
	    }
	}
    }
  else
    printf ("(nil pattern)\n");
}


/*
static void phases_mark_first (phase *p)
// Helperfunction: search the first living cell (starting with (left, top)) and set firstX/Y accordingly
{
  assert (p);
  assert (p->pat);
  assert (p->pat->cell);
  assert (p->pat->cell [0]);
  assert (p->pat->top == 0);
  assert (p->pat->left == 0);
  assert (W(p->pat) > 0);
  assert (H(p->pat) > 0);

  p->firstY = p->pat->top + 1;
  for (p->firstX = p->pat->left; p->firstX < p->pat->right; p->firstX++)
    if (p->pat->cell [p->firstY][p->firstX] == ALIVE)
      return;

  // we're not supposed to end here ...
  assert (0);
}


void phases_load (object *o, const char *name, const char *pat_str)

{
  int i;

  pat_from_string (pat_str);
  pat_envelope (&lab [0]);
  o->phases [0].pat = pat_compact (&lab [0], NULL);
  o->phases [0].offX = 0;
  o->phases [0].offY = 0;
  phases_mark_first (&o->phases [0]);

  for (i = 1; i < o->nrPhases; i++)
    {
      pat_generate (&lab [i-1], &lab [i]);
      if (!W(&lab[i]))
	{
	  fprintf (stderr, "object '%s' dies after %d phases!\n", name, i);
	  exit (1);
	}
      pat_envelope (&lab [i]);
      o->phases [i].pat = pat_compact (&lab [i], NULL);
      o->phases [i].offX = lab [i].left - lab [0].left;
      o->phases [i].offY = lab [i].top - lab [0].top;
      phases_mark_first (&o->phases [i]);
    }
}


void phases_derive (object *v1, pat_xform pxf, object *o)

{
  int i, p0left = 0, p0top = 0;

  o->nrPhases = v1->nrPhases;
  ALLOC(phase,o->phases,o->nrPhases)

  for (i = 0; i < o->nrPhases; i++)
    {
      pattern *temp = pat_apply_xform (&lab [i], pxf);
      if (!i)
	{
	  p0left = temp->left;
	  p0top = temp->top;
	}

      o->phases [i].pat = pat_compact (temp, NULL);
      o->phases [i].offX = temp->left - p0left;
      o->phases [i].offY = temp->top - p0top;
      phases_mark_first (&o->phases [i]);
      free (temp);
    }
}


void objgrp_match (objectGroup *og, int g)
// Try to find the objects in *og in generation g
// Copy all hits into og->foundObjects [] and calculate og->cost as the sum of all found objects.

{
  int i, j, x, y, t, l, b, r, dx, dy;

  // First: initialize og, assuming we did'nt find nothing.
  og->cost = 0;
  og->nrFoundObjects = 0;

  // Try all our objects
  for (i = 0; i < og->nrObjectRefs; i++)
    {
      object *o = og->objectRefs [i].obj;

      // Try all phases of the current object.
      for (j = 0; j < o->nrPhases; j++)
	{
	  phase *ph = &o->phases [j];
	  pattern *pat = ph->pat;

	  assert (pat->top == 0);
	  assert (pat->left == 0);

	  // Take the following into account:
	  //        - both pattern and obj->pat are surronded by a frame of non-ALIVE cells.
	  //        - Checking for objects outside of the pattern is pointless and even dangerous (segfault anyone?)
	  t = lab [g].top + (ph->firstY-1);
	  l = lab [g].left + (ph->firstX-1);
	  b = lab [g].bottom - (pat->bottom - 1 - ph->firstY);
	  r = lab [g].right - (pat->right - 1 - ph->firstX);
	  for (y = t; y <= b; y++)
	    for (x = l; x <= r; x++)
	      if (lab [g].cell [y][x] == ALIVE)
		{
		  int OK = true;
		  for (dy = 0; OK && dy <= pat->bottom; dy++)
		    for (dx = 0; OK && dx <= pat->right; dx++)
		      if (pat->cell [dy][dx] != UNDEF && pat->cell [dy][dx] != lab [g].cell [y-ph->firstY+dy][x-ph->firstX+dx])
			OK = false;

		  // Do we have a match? If yes: keep it!
		  if (OK)
		    {
		      if (og->nrFoundObjects >= MAX_FIND)
			{
			  fprintf (stderr, "Internal error, please increase MAX_FIND!\n");
			  exit (3);
			}

		      og->foundObjects [og->nrFoundObjects  ].what = o;
		      og->foundObjects [og->nrFoundObjects  ].minGeneration = g;
		      og->foundObjects [og->nrFoundObjects  ].maxGeneration = g;
		      og->foundObjects [og->nrFoundObjects  ].foundGen = g;
		      og->foundObjects [og->nrFoundObjects  ].foundX = x - o->phases [j].offX - ph->firstX + 1;
		      og->foundObjects [og->nrFoundObjects  ].foundY = y - o->phases [j].offY - ph->firstY + 1;
		      og->foundObjects [og->nrFoundObjects++].foundPhase = j;
		      og->cost += og->objectRefs [i].cost;
		    }
		}
	}
    }
}


bool phase_match (phase *ph, int g, int x, int y)

{
  int OK = true, dx, dy;
  pattern *pat = ph->pat;

  x += ph->offX;
  y += ph->offY;
  for (dy = 0; OK && dy <= pat->bottom; dy++)
    for (dx = 0; OK && dx <= pat->right; dx++)
      if (pat->cell [dy][dx] != UNDEF && pat->cell [dy][dx] != lab [g].cell [y-1+dy][x-1+dx])
	OK = false;

  return OK;
}


void objgrp_trace (objectGroup *og, direction dir)

{
  int g, i, j, p;
  object *o;

  switch (dir)
    {
      case dir_forward:
	for (i = 0; i < og->nrFoundObjects; i++)
	  {
	    o = og->foundObjects [i].what;
	    j = 0;
	    p = og->foundObjects [i].foundPhase;
	    for (g = og->foundObjects [i].foundGen; g < maxGen; g++, p++)
	      {
		if (p >= o->nrPhases)
		  {
		    p = 0;
		    j++;
		  }
		if (phase_match (&o->phases [p], g, og->foundObjects [i].foundX + j*o->dx, og->foundObjects [i].foundY + j*o->dy))
		  og->foundObjects [i].maxGeneration = g;
		else
		  break;
	      }
	  }
	break;
      case dir_back:
	for (i = 0; i < og->nrFoundObjects; i++)
	  {
	    o = og->foundObjects [i].what;
	    j = 0;
	    p = og->foundObjects [i].foundPhase;
	    for (g = og->foundObjects [i].foundGen; g >= 0; g--, p--)
	      {
		if (p < 0)
		  {
		    p = o->nrPhases-1;
		    j++;
		  }
		if (phase_match (&o->phases [p], g, og->foundObjects [i].foundX - j*o->dx, og->foundObjects [i].foundY - j*o->dy))
		  og->foundObjects [i].minGeneration = g;
		else
		  break;
	      }
	  }
	break;
      //case dir_undef:
      default:
	assert (0);
    }
}
*/


bool pat_compare (pattern *p1, pattern *p2)
// return true iff p1 an p2 are the same GoL pattern.

{
  int x, y;

  // Different size / location -> differnt pattern!
  if (p1->top != p2->top)
    return false;
  if (p1->bottom != p2->bottom)
    return false;
  if (p1->left != p2->left)
    return false;
  if (p1->right != p2->right)
    return false;

  // Copy ALL cells! even dead ones.
  for (y = p1->top; y <= p1->bottom;  y++)
    for (x = p1->left; x <= p1->right;  x++)
      if ((p1->cell [y][x] == ALIVE) != (p2->cell [y][x] == ALIVE))
	return false;

  // If we get here, we didn't find no difference!
  return true;
}
