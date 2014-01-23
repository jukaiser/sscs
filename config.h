/* Customizable parts of the search program */

/* Maximum search depth in terms of total cost for the salvo.
   Changing this will slightly change the amount of memory needed by queue.c (there is an static void * qs [MAXCOST])
*/
#define MAXCOST	8192

/* Dimensions of construction site

   Slightly affects memory consumption and search speed.
   (Something like MAXWIDTH*MAXHEIGHT*(MAXGEN+2) int's are needed for the evaluation of any reaction).
   The calculation is clipped to the active area of the reaction.
*/
#define MAXWIDTH 64
#define MAXHEIGHT 64
#define MAXGEN	256
