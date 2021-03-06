/* parse a simple .cfg file to a set of global parameters */
#include <string.h>
#include <ctype.h>

#include "config.h"


// -> config.h!
char *PATH	  = "test";
char *F_TEMPFILES = "tmp/%02u/chunk-%04u-%09u.raw";
char *SAVEFILE	  = "tmp/status.save";
int   MAXCOST	  = 8192;
int   MAXWIDTH	  = 128;
int   MAXHEIGHT	  = 64;
int   PRUNE_SX	  = 100;
int   PRUNE_SY	  = 34;
int  *SHIP_DIRS   = NULL;
int  nSHIP_DIRS   = 0;
int   MAXGEN	  = 256;
int   MAXPERIOD   = 2;
int   MAX_RLE	  = 4096;
char *SHIPNAME	  = "maship";
char *START	  = "start-targets.pat";
int   MAX_FIND	  = 100;
bool  SHIPMODE    = true;
int   DX	  = 0;
int   DY	  = -31;
int   DT	  = 240;
int   FACTOR	  = 1;
int   LANES	  = 31;
bool  RELATIVE    = true;
int   MAXDELTA	  = 31;
char *DBHOST	  = "localhost";
int   DBPORT	  = 3306;
char *DBNAME	  = "gol";
char *DBUSER	  = "gol";
char *DBPASSWD    = "";
char *SQL_F_FETCH_TARGET = "SELECT rle, period FROM target WHERE tId = %llu";
char *SQL_F_SEARCH_TARGET = "SELECT tId FROM target WHERE hashVal = %u AND rle = '%s'";
char *SQL_F_STORE_TARGET = "INSERT INTO target (hashVal, rle, period, combined_width, combined_height) VALUES (%u, '%s', %u, %u, %u)";
char *SQL_F_IS_FINISHED_REACTION = "SELECT rId, min_cost FROM reaction WHERE initial_tId = %llu AND initial_phase = %u AND bId = %llu AND lane = %u";
char *SQL_F_STORE_REACTION =
	"INSERT INTO reaction (initial_tId, initial_phase, bId, lane, lane_adj, result_tId, result_phase, offX, offY, delay, gen, min_cost, result, emits_ships) "
		"VALUES (%llu, %u, %llu, %u, %d, %llu, %u, %d, %d, %u, %u, %u, '%s', '%s')";
char *SQL_F_STORE_TRANSITION =
	"INSERT INTO transition (rId, initial_state, result_state, rephase, pId, cost, total_cost) "
		"VALUES (%llu, %u, %u, %u, %llu, %u, %u)";
char *SQL_F_STORE_EMIT = "INSERT DELAYED INTO emitted (rId, seq, oId, offX, offY, gen) VALUES (%llu, %u, %llu, %d, %d, %d)";
char *SQL_COUNT_SPACESHIPS = "SELECT COUNT(*) FROM object WHERE dx <> 0 OR dy <> 0";
char *SQL_F_SPACESHIPS = "SELECT oId, rle, name, dx, dy, dt, phase, offX, offY FROM object WHERE dx <> 0 OR dy <> 0 ORDER BY name, phase";
char *SQL_COUNT_BULLETS = "SELECT COUNT(DISTINCT bId) FROM part WHERE ship_name = '%s'";
char *SQL_F_BULLETS =
	"SELECT b.bId, b.oId, rle, dx, dy, dt, base_x, base_y, reference, lane_dx, lane_dy, lanes_per_height, lanes_per_width, extra_lanes "
		"FROM bullet b LEFT JOIN object USING (oId) WHERE b.bId IN (SELECT bId FROM part WHERE ship_name = '%s')";
char *SQL_COUNT_PARTS = "SELECT COUNT(*) FROM part WHERE ship_name = '%s'";
char *SQL_F_PARTS = "SELECT pId, part_name, type, lane_adjust, bId, lane_fired, cost FROM part WHERE ship_name = '%s'";
char *SQL_F_PARTS_COMPLETE = "SELECT pId, part_name, type, lane_adjust, bId, lane_fired, cost, rle, dx, dy, offX, offY, fireX, fireY FROM part WHERE ship_name = '%s'";


// <- config.h

typedef enum {NUM, STRING, BOOL, N_ARRAY} cfg_type;
typedef struct
  {
    char *name;
    cfg_type type;
    void *v_ptr;
    int *n_ptr;
  } cfg_var;

static cfg_var config [] =
  {
    {"PATH",			   STRING,  &PATH},
    {"F_TEMPFILES",		   STRING,  &F_TEMPFILES},
    {"SAVEFILE",		   STRING,  &SAVEFILE},
    {"MAXCOST",			   NUM,     &MAXCOST},
    {"MAXWIDTH",		   NUM,     &MAXWIDTH},
    {"MAXHEIGHT",		   NUM,     &MAXHEIGHT},
    {"PRUNE_SX",		   NUM,     &PRUNE_SX},
    {"PRUNE_SY",		   NUM,     &PRUNE_SY},
    {"SHIP_DIRS",		   N_ARRAY, &SHIP_DIRS, &nSHIP_DIRS},
    {"MAXGEN",			   NUM,     &MAXGEN},
    {"MAXPERIOD",		   NUM,     &MAXPERIOD},
    {"MAX_RLE",			   NUM,     &MAX_RLE},
    {"SHIPNAME",		   STRING,  &SHIPNAME},
    {"START",			   STRING,  &START},
    {"MAX_FIND",		   NUM,     &MAX_FIND},
    {"SHIPMODE",		   BOOL,    &SHIPMODE},
    {"DX",			   NUM,     &DX},
    {"DY",			   NUM,     &DY},
    {"DT",			   NUM,     &DT},
    {"FACTOR",			   NUM,     &FACTOR},
    {"LANES",			   NUM,     &LANES},
    {"RELATIVE",		   BOOL,    &RELATIVE},
    {"MAXDELTA",		   NUM,     &MAXDELTA},
    {"DBHOST",			   STRING,  &DBHOST},
    {"DBPORT",			   NUM,     &DBPORT},
    {"DBNAME",			   STRING,  &DBNAME},
    {"DBUSER",			   STRING,  &DBUSER},
    {"DBPASSWD",		   STRING,  &DBPASSWD},
    {"SQL_F_FETCH_TARGET",	   STRING,  &SQL_F_FETCH_TARGET},
    {"SQL_F_SEARCH_TARGET",	   STRING,  &SQL_F_SEARCH_TARGET},
    {"SQL_F_STORE_TARGET",	   STRING,  &SQL_F_STORE_TARGET},
    {"SQL_F_IS_FINISHED_REACTION", STRING,  &SQL_F_IS_FINISHED_REACTION},
    {"SQL_F_STORE_REACTION",	   STRING,  &SQL_F_STORE_REACTION},
    {"SQL_F_STORE_EMIT",	   STRING,  &SQL_F_STORE_EMIT},
    {"SQL_COUNT_SPACESHIPS",	   STRING,  &SQL_COUNT_SPACESHIPS},
    {"SQL_F_SPACESHIPS",	   STRING,  &SQL_F_SPACESHIPS},
    {"SQL_COUNT_BULLETS",	   STRING,  &SQL_COUNT_BULLETS},
    {"SQL_F_BULLETS",		   STRING,  &SQL_F_BULLETS},
    {"SQL_COUNT_PARTS",		   STRING,  &SQL_COUNT_PARTS},
    {"SQL_F_PARTS_COMPLETE",	   STRING,  &SQL_F_PARTS_COMPLETE},
    {"SQL_F_PARTS",		   STRING,  &SQL_F_PARTS},
    {NULL}
  };

typedef struct
  {
    char *name;
    int  value;
  } named_const;

static named_const constants [] =
  {
    {"TRUE",      (int)true},
    {"ON",        (int)true},
    {"FALSE",     (int)false},
    {"OFF",       (int)false},
    {"STILL",     1},
    {"N",         (int) dir_north},
    {"NORTH",     (int) dir_north},
    {"NE",        (int) dir_northeast},
    {"NORTHEAST", (int) dir_northeast},
    {"E",         (int) dir_east},
    {"EAST",      (int) dir_east},
    {"SE",        (int) dir_southeast},
    {"SOUTHEAST", (int) dir_southeast},
    {"S",         (int) dir_south},
    {"SOUTH",     (int) dir_south},
    {"SW",        (int) dir_southwest},
    {"SOUTHWEST", (int) dir_southwest},
    {"W",	  (int) dir_west},
    {"WEST",	  (int) dir_west},
    {"NW",        (int) dir_north_west},
    {"NORTHWEST", (int) dir_north_west},
    {NULL}
  };


static int parse_constant (const char *var, const char **val, bool isArray)

{
  char buf [11];
  int  i = 0;
  named_const *c;

  while (**val && isalnum (**val) && i < 10)
    buf [i++] = toupper (*((*val)++));
  buf [i] = '\0';

  if (!isArray && **val)
    fprintf (stderr, "config: Variable '%s': ignoring trailing junk after value (%s)!\n", var, buf);

  for (c = constants; c->name; c++)
    if (strcmp (c->name, buf) == 0)
      return c->value;

  fprintf (stderr, "config: Variable '%s': constant %s unknown!\n", var, buf);
  return 0; // whatever.
}


char *skip_ws (char *cp)

{
  while (*cp && isspace (*cp))
    cp++;

  return cp;
}


static void handle_num (const char *var, int *v_ptr, const char *val)

{
  char *end;

  if (*val == 'p' || *val == 'P')
    val++;
  else if (isalpha (*val))
    {
      *v_ptr = parse_constant (var, &val, false);
      return;
    }

  *v_ptr = strtol (val, &end, 0);
  if (*end)
    fprintf (stderr, "config: Variable '%s': ignoring trailing junk '%s' after value (%d)!\n", var, end, *v_ptr);
}


static void handle_bool (const char *var, bool *v_ptr, const char *val)

{
  char *end;

  if (*val == 'p' || *val == 'P')
    val++;
  else if (isalpha (*val))
    {
      *v_ptr = parse_constant (var, &val, false);
      return;
    }

  *v_ptr = strtol (val, &end, 0);
  if (!*end)
    fprintf (stderr, "config: Variable '%s': ignoring trailing junk after value (%d)!\n", var, *v_ptr);
  *v_ptr = (strtol (val, NULL, 10) != 0);
}


static void handle_string (const char *var, char **v_ptr, const char *val)

{
  *v_ptr = strdup (val);
  if (!v_ptr)
    {
      perror ("config::handle_string() - strdup");
      exit (2);
    }
}


static void handle_array (const char *var, int **v_ptr, int *n_ptr, char *val)

{
  const char *end;

  // special case: cost vector might be defined by '[' step '%' nLanes ']'
  // Idea: there is only one way to change lanes, and step is defining the step size of those changes.
  // So to reach line N you have to find an k <= nLanes with N = (k * step) % nLanes
  // Since you need k steps to reach the new lane, that is our cost ;)
  if (*val == '[')
    {
      int step, nLanes, k;

      step = strtol (val+1, &val, 0);
      if (step <= 0)
	{
	  fprintf (stderr, "config: Variable '%s': illeagal use of [9 %% 31] notation ->%s!\n", var, val);
	  return;
	}
      val = skip_ws (val);

      // We expect an '%' here!
      if (*val != '%')
	{
	  fprintf (stderr, "config: Variable '%s': illeagal use of [9 %% 31] notation ->%s!\n", var, val);
	  return;
	}

      nLanes = strtol (val+1, &val, 0);
      if (nLanes <= step)
	{
	  fprintf (stderr, "config: Variable '%s': illeagal use of [9 %% 31] notation ->%s!\n", var, val);
	  return;
	}
      val = skip_ws (val);

      // We expect an ']' here!
      if (*val != ']')
	{
	  fprintf (stderr, "config: Variable '%s': illeagal use of [9 %% 31] notation ->%s!\n", var, val);
	  return;
	}
      if (val [1])
	fprintf (stderr, "config: Variable '%s': ignoring trailing garbage ('%s')!\n", var, val+1);

      // Ok ... parsing was successful!
      ALLOC(int,*v_ptr,nLanes)
      for (k = 1; k <= nLanes; k++)
	(*v_ptr) [(k*step)%nLanes] = k;
      *n_ptr = nLanes;

      return;
    }

  // hard to know how many elements we'll end up with. Better safe then sorry ...
  // since we need atleast one WS between to values 1+strlen()/2 should be fine.
  ALLOC(int,*v_ptr,1+strlen (val)/2)

  // Parse each value into a number ...
  *n_ptr = 0;
  end = val;
  while (*end)
    {
      if (isalpha (*val))
	(*v_ptr) [(*n_ptr)++] = parse_constant (var, &end, true);
      else if (isdigit (*end))
        (*v_ptr) [(*n_ptr)++] = strtol (end, (char **)&end, 0);
      else
	{
	  fprintf (stderr, "config: Variable '%s': ignoring trailing junk '%s' after value (%d)!\n", var, end, *v_ptr);
	  return;
	}
      end = skip_ws ((char *)end);
    }
}


void config_load (const char *cfg_name)

{
  FILE *cfg = fopen (cfg_name, "rt");
  char buffer [4096], var [4096], *val;
  cfg_var *cv;

  if (!cfg)
    {
      perror (cfg_name);
      return;
    }

  while (fgets (buffer, 4096, cfg))
    {
      // Kill comments
      char *split = strchr (buffer, '#');
      char *cp1 = var, *cp2 = buffer;
      if (split) *split = '\0';

      // remove trailing newline
      split = strrchr (buffer, '\n');
      if (split) *split = '\0';

      // Skip leading white space ...
      cp2 = skip_ws (cp2);

      // Silently ignore empty lines.
      if (!*cp2)
	continue;

      // search for '='
      val = strchr (cp2, '=');
      if (!val)
	{
	  fprintf (stderr, "%s: ignoring bad config line starting \"%s\" (missing '='!)\n", cfg_name, buffer);
	  continue;
	}
      *val++ = '\0';

      // front part basically must be a variable name.
      // To keep it simple we ignore *all* white space here - even if inside the variable name.
      while (*cp2)
	{
	  if (!isspace (*cp2))
	    *cp1++ = toupper (*cp2);
	  *cp2++;
	}
      *cp1 = '\0';

      // remove leading and trailing white space off val
      val = skip_ws (val);
      if (!*val)
	{
	  fprintf (stderr, "%s: ignoring bad config line starting \"%s\" (missing value!)\n", cfg_name, buffer);
	  continue;
	}
      cp2 = strchr (val, '\0') - 1;
      while (cp2 > val && isspace (*cp2))
	cp2--;
      cp2 [1] = '\0';

      // search the variable
      for (cv = config; cv->name; cv++)
	if (strcmp (cv->name, var) == 0)
	  break;
      if (!cv->name)
	fprintf (stderr, "%s: ignoring bad config line starting \"%s\" (variable '%s' unknown!)\n", cfg_name, buffer, var);
      else switch (cv->type)
	{
	  case NUM:
		handle_num (var, cv->v_ptr, val);
		break;
	  case BOOL:
		handle_bool (var, cv->v_ptr, val);
		break;
	  case STRING:
		handle_string (var, cv->v_ptr, val);
		break;
	  case N_ARRAY:
		handle_array (var, cv->v_ptr, cv->n_ptr, val);
		break;
	}
    }

  fclose (cfg);
}
