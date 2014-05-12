/* ship generator (fills DB Table space_ships */

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "pattern.h"


#define S_FMT "../gencols/obj/%s.life"
#define P_FMT "parts/31c240/%s.rle"


pattern *t, *temp;


typedef struct
  {
    char *name;
    char *pat;
    int  dx, dy, dt;
  } ship;

typedef struct
  {
    char *fname;
    char *name;
    char *type;
    unsigned dx, dy;
    int offX, offY, fireX, fireY;
    int lane_adj;
    int lane_fired;
    int cost;
  } part;


char *ships [] =
  {
    "glider_ne", "glider_nw", "glider_se", "glider_sw",
    "hwss_e", "hwss_n", "hwss_s", "hwss_w",
    "lwss_e", "lwss_n", "lwss_s", "lwss_w",
    "mwss_e", "mwss_n", "mwss_s", "mwss_w"
  };

part parts [] =
  {
    {"T_o0x0_delta31",			"T",      "track",    0, 31,  0,   0, 0,   0,   0,  0,  0},
    {"R1_o-35x0_delta270",		"R1",     "rephaser", 0, 270, -35, 0, 0,   0,   9,  0,  1},
    {"R1L0_o-25x0_delta270_f738x288",	"R1L0B",  "rake",     0, 270, -25, 0, 738, 288, 9,  0,  1},
    {"faked",				"R1L2B",  "rake",     0, 0, 0, 0, 0, 0,         9,  2,  1},
    {"R2L7_o-39x0_delta695_f738x932",	"R2L7B",  "rake",     0, 695, -39, 0, 738, 932, 18, 7,  2},
    {"R3L19_o-43x0_delta965_f738x331",	"R3L19B", "rake",     0, 965, -43, 0, 738, 331, 27, 19, 3},
    {"R3L26_o-53x0_delta996_f738x944",	"R3L26B", "rake",     0, 996, -53, 0, 738, 994, 27, 26, 3},
    {"faked",				"R3L4B",  "rake",     0, 0, 0, 0, 0, 0,         27, 4,  3},
    {"faked",				"R4L14B", "rake",     0, 0, 0, 0, 0, 0,         5,  14, 4},
    {"faked",				"R5L21B", "rake",     0, 0, 0, 0, 0, 0,         14, 21, 5}
  };

main (int argc, char **argv)

{
  char fname [1024], *rle;
  FILE *f;
  int i, j;

  lab_allocate (20, 20, 5, 2);

  for (i = 0; i < (sizeof (ships) / sizeof (char*)); i++)
    {
      sprintf (fname, S_FMT, ships [i]);
      f = fopen (fname, "r");
      if (!f) { perror (fname); exit (2); }

      lab_init ();
      pat_load (f);

      for (j = 0; j < 4; j++)
	pat_generate (&lab [j], &lab [j+1]);

      for (j = 0; j < 4; j++)
	{
	  rle = pat_rle (&lab [j]);
	  printf ("INSERT INTO object VALUES (NULL, '%s', '%s', %d, %d, %d, %d, 4, %d, %d, %d);\n",
		  rle, ships [i], W(&lab[j]), H(&lab[j]), lab[4].left-lab[0].left, lab[4].top-lab[0].top, j, lab[j].left-lab[0].left, lab[j].top-lab[0].top);
	  if (strcmp (rle, "o$b2o$2o!") == 0)
	    printf ("INSERT INTO bullet VALUES (1,'gl-se',LAST_INSERT_ID(),-4,0,'BOTTOMLEFT',0,-1,1,1,8);\n");
	}
    }

  for (i = 0; i < (sizeof (parts) / sizeof (part)); i++)
    {
      sprintf (fname, P_FMT, parts [i].fname);
      f = fopen (fname, "r");
      if (!f) { perror (fname); exit (2); }

      printf ("INSERT INTO part VALUES (NULL, \n");
      while (!feof (f))
	{
	  char buf [81], *nl;
	  if (!fgets (buf, 80, f))
	    {
	      if (feof (f))
		break;
	      perror (fname);
	      exit (2);
	    }
	  if (!buf [0] || buf [0] == 'x')
	    continue;
	  nl = strrchr (buf, '\n');
	  if (nl) *nl = '\0';
	  printf ("'%s'\n", buf);
	}
      printf (", '31c240', '%s', %u, %u, %d, %d, %u, %u, '%s', %d, %s, %d, %d);\n\n", parts [i].name, parts [i].dx, parts [i].dy, parts [i].offX, parts [i].offY,
	      parts [i].fireX, parts [i].fireY, parts [i].type, parts [i].lane_adj, strcmp (parts[i].type, "rake") == 0 ? "1": "NULL", parts [i].lane_fired, parts [i].cost);
    }
}
