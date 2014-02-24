/* define and handle reactions */

typedef struct
   {
     ROWID   id;
     int     cost;
     guarded *g_tgt;
     bullet *bullet;
     int     lane;
   } reaction;


// better naming plx
void build_reactions (int nph, bullet *b, bool preload, int old_cost, int old_lane);
void build_targets (int start, int nph);
void handle (reaction *r);
void init_reactions (void);
