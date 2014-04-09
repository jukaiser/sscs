/* Customizable parts of the search program */

#include <stdbool.h>


typedef enum {dir_north, dir_northeast, dir_east, dir_southeast, dir_south, dir_southwest, dir_west, dir_north_west} ship_dirs;

extern char *PATH;		// Where to look for files
extern char *F_TEMPFILES;	// Format for temporary file names
extern char *SAVEFILE;		// File for saving information needed for contiuing/extending a finished search

extern int   MAXCOST;		// Maximum cost of reaction chains we will look at.

/* Context of our pattern generating alg. How big is the universe, how many generations to look at. Max. period of oscillators */
extern int   MAXWIDTH;
extern int   MAXHEIGHT;
extern int   MAXGEN;
extern int   MAXPERIOD;

extern int   MAX_RLE;		// Max size of any RLE that we can encode.
extern char *SHIPNAME;		// Name of the ship. Needed to load it's parts
extern char *START;		// Where do we find the first targets? (filename)
extern int   MAX_FIND;		// Maximum number of sub patterns to detect before pattern analyzer calls it quits.

/* SHIPMODE == true => we are looking at shipparts, not a (relativly) static constructing device
   In ship mode the construction is assumed to be done by rakes traveling at (dx,dy)c/dt of period dt*factor.
   It is further assumed, that the construction is repeated every dt*factor generations at another site (dx*factor,dy*factor) cells away.
   I.e.: the target we are looking at is repeated "down ship" of us, while "up ship" there is the result of our target hit by the current bullet on the current lane (after dt*factor ticks)
*/
extern bool  SHIPMODE;
extern int   DX;
extern int   DY;
extern int   DT;
extern int   FACTOR;

extern int   PRUNE_SX;		// max. size of a resulting target for further consideration.
extern int   PRUNE_SY;
extern int  *SHIP_DIRS;		// allowable ship directions
extern int  nSHIP_DIRS;

extern int   LANES;		// Number of possible Lanes to look at
extern bool  RELATIVE;		// Normally RELATVIE will be true. (both elbow movements and rake rephasings are based on the last lane used)
extern int  MAXDELTA;		// maximum value in COSTS

/* Database interace */
extern char *DBHOST;		// the usual connection parameters for a mysql db. DBUSER must exist and have the GRANTs needed by us.
extern int   DBPORT;
extern char *DBNAME;
extern char *DBUSER;
extern char *DBPASSWD;

/* SQL templates for accessing the above defined database
   For each table we need a set of three templates: SQL_INSERT_*, SQL_FETCH_*, SQL_UNIQUE_*
   They are used for storing and retreiving data to and from the table - and for checking if an object is already stored there.
*/
extern char *SQL_F_FETCH_TARGET;
extern char *SQL_F_SEARCH_TARGET;
extern char *SQL_F_STORE_TARGET;
extern char *SQL_F_FETCH_REACTION;
extern char *SQL_F_IS_FINISHED_REACTION;
extern char *SQL_F_STORE_REACTION;
// extern char *SQL_F_REACTION_EMITS;
// extern char *SQL_F_FINISH_REACTION;
extern char *SQL_F_STORE_EMIT;
extern char *SQL_COUNT_SPACESHIPS;
extern char *SQL_F_SPACESHIPS;
extern char *SQL_COUNT_BULLETS;
extern char *SQL_F_BULLETS;
extern char *SQL_COUNT_PARTS;
extern char *SQL_F_PARTS;

void config_load (const char *cfg_name);
