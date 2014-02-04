#define DEAD	'.'
#define ALIVE	'*'
#define	UNDEF	' '
#define VISITED	':'

// pattern and stuff
typedef struct
  {
    int top, bottom, left, right;	// bounding box
    int sizeX, sizeY;			// actual size of cell [][]
    char **cell;			// 2dim array of cells (cell [Y][X] == ALIVE ...)
  } pattern;


typedef enum {topleft, topright, bottomleft, bottomright} corner;
typedef struct
  {
    pattern *p;
    int     dx, dy, dt;			// speed and period of the bullet
    int     base_x, base_y;		// Where will lane 0 be?
    corner  reference;
    int     lane_dx, lane_dy;		// How to iterate lanes.
    int     lanes_per_width;
    int	    lanes_per_height;
    int	    extra_lanes;
  } bullet;


extern pattern *lab;

pattern *pat_allocate (pattern *p, int sx, int sy);
void pat_init (pattern *p);
void pat_fill (pattern *p, char value);
void pat_deallocate (pattern *pat);
pattern *pat_compact (pattern *p1, pattern *p2);
void pat_shrink_bbox (pattern *p);
bool pat_generate (pattern *p1, pattern *p2);
void pat_add (pattern *p1, int X, int Y, pattern *p2);
void pat_copy (pattern *p1, int offX, int offY, pattern *p2);
void pat_remove (pattern *p1, int offX, int offY, pattern *p2);
void pat_load (FILE *f);
void pat_from_string (const char *str);
char *pat_rle (pattern *pat);
bool pat_compare (pattern *p1, pattern *p2);

void lab_allocate (int _maxX, int _maxY, int _maxGen);
void lab_init (void);

void pat_collide (pattern *target, bullet *b, int lane);
int  pat_count_lanes (pattern *target, bullet *b);

#define W(p)    ((p)->right-(p)->left+1)
#define H(p)    ((p)->bottom-(p)->top+1)
