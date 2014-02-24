/* define and handle reactions */

typedef struct
   {
     ROWID   rId;
     ROWID   tId;
     bullet *bullet;
     int     lane;
     int     cost, delta;
   } reaction;


// better naming plx
void build_reactions (int nph, bullet *b, bool preload, int old_cost, int old_lane);
void build_targets (int start, int nph);
void free_targets (void);
void handle (reaction *r);
void init_reactions (void);
