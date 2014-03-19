/* define and handle reactions */

typedef struct
  {
    ROWID    rId;
    ROWID    tId;
    unsigned cost;
    uint8_t  delta;
    uint8_t  b;
    uint8_t  lane;
  } reaction;
  
typedef enum {rt_undef, rt_dies, rt_flyby, rt_stable, rt_pruned, rt_unfinished} reaction_type;

typedef struct
  {
    ROWID	  result_tId;
    int		  offX;
    int		  offY;
    int		  gen;
    bool	  emits;
    reaction_type type;
  } result;

// better naming plx
void build_reactions (int nph, int b, bool preload, unsigned old_cost, int old_lane);
void build_targets (int start, int nph);
void reaction_targets_keep (int nph);
void free_targets (void);
void handle (reaction *r);
void init_reactions (void);
