#define DEAD	'.'
#define ALIVE	'*'
#define	UNDEF	' '
#define VISITED	':'

// WARNING: you must fit this to the data type your DB uses for ROW IDs.
typedef uint64_t ROWID;

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
    ROWID   id;				// ROW ID in database table
    char    *name;			// name to access/reference this bullet.
    ROWID   oId;			// id of the corresponding object
    pattern *p;
    int     dx, dy, dt;			// speed and period of the bullet
    int     base_x, base_y;		// Where will lane 0 be?
    corner  reference;
    int     lane_dx, lane_dy;		// How to iterate lanes.
    int     lanes_per_width;
    int	    lanes_per_height;
    int	    extra_lanes;
  } bullet;

typedef struct
  {
    ROWID    id;			// ROW ID in database table (0 if not yet stored!)
    pattern *pat;			// actual pattern to use as target
    int      nph;			// period of pattern
    int      top, bottom, left, right;	// combined bbox of all phases of this target
    int      X, Y;			// position of *this* phase
  } target;

typedef struct
  {
    ROWID    id;			// ROW ID in database table
    pattern *pat;
    char    *name;
    int      firstX, firstY;		// coord's of first ALIVE pixel in pat
    int      dx, dy, dt;		// speed and period of the object (if applicable)
					// dx=dy=0, dt=1 -> still life, dx=dy=0, dt>1 osci
    bool     wanted;			// if wanted == false: prune current reaction!
  } object;

typedef struct
  {
    object *obj;			// WHAT has been found?
    int    offX, offY;			// WHERE?
    int    gen;				// WHEN?
  } found;

extern pattern *lab;
extern bullet bullets [1];

pattern *pat_allocate (pattern *p, int sx, int sy);
void pat_init (pattern *p);
void pat_fill (pattern *p, char value);
void pat_dump (pattern *p, bool withHeader);
void pat_deallocate (pattern *pat);
void pat_envelope (pattern *pat);
pattern *pat_compact (pattern *p1, pattern *p2);
void pat_shrink_bbox (pattern *p);
bool pat_generate (pattern *p1, pattern *p2);
void pat_add (pattern *p1, int X, int Y, pattern *p2);
void pat_copy (pattern *p1, int offX, int offY, pattern *p2);
bool pat_match (pattern *p1, int offX, int offY, pattern *p2);
void pat_remove (pattern *p1, int offX, int offY, pattern *p2);
void pat_load (FILE *f);
void pat_from_string (const char *str);
bool pat_touches_border (pattern *p, int dist);
char *pat_rle (pattern *pat);
bool pat_compare (pattern *p1, pattern *p2);
void obj_mark_first (object *p);
found *obj_search (int gen, object *objs, int *n);
int obj_back_trace (void);

void lab_allocate (int _maxX, int _maxY, int _maxGen, int _maxFind);
void lab_init (void);

void tgt_center (target *tgt);
void tgt_collide (const target *const tgt, bullet *b, int lane, int *fly_x, int *fly_y, int *fly_dt);
int  tgt_count_lanes (const target *const tgt, int _b);
int  tgt_adjust_lane (int _b, target *old, target *new);

#define W(p)    ((p)->right-(p)->left+1)
#define H(p)    ((p)->bottom-(p)->top+1)
