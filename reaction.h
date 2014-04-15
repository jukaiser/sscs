/* define and handle reactions */

typedef struct
  {
    ROWID   tId;
    uint8_t phase;
    uint8_t b;
    uint8_t lane;
    uint8_t state;
    uint8_t cost;
  } reaction;

typedef enum {pt_track, pt_rephaser, pt_rake} part_type;
typedef struct
  {
    // For the search program we only need some fields of the DB table, the rest is only needed for stitching stuff up.
    ROWID	  pId;
    char	  *name;	// part.part_name 
    part_type	  type;
    unsigned	  lane_adjust;
    unsigned	  lane_fired;
    int		  b;		// index in bullets [] for part.bId
    unsigned	  cost;
  } part;

typedef enum {rt_undef, rt_dies, rt_flyby, rt_stable, rt_pruned, rt_unfinished} reaction_type;
typedef struct
  {
    ROWID	  result_tId;
    int		  result_phase;
    int		  offX;
    int		  offY;
    int		  gen;
    int		  delay;
    int		  lane_adj;
    bool	  emits;
    reaction_type type;
  } result;

// better naming plx
void build_reactions (int nph, int b, bool preload, uint8_t old_cost, uint8_t state);
void build_targets (int start, int nph);
void reaction_targets_keep (int nph);
void free_targets (void);
void handle (reaction *r);
void init_reactions (void);
