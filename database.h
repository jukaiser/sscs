/* Database driver - replace mysql/mariadb with your DB, if you like */

void db_init (void);

void db_bullet_load (const char *name, bullet *b);
object *db_load_space_ships (void);

typedef enum {dbrt_undef, dbrt_dies, dbrt_flyby, dbrt_stable, dbrt_pruned, dbrt_unfinished} db_reaction_type;

void db_target_store (target *tgt, const char *rle);
void db_target_link (ROWID curr, ROWID nxt);
bool db_target_lookup (target *tgt, const char *rle);
void db_target_fetch (reaction *r, target *t);
bool db_reaction_keep (reaction *r);
bool db_is_reaction_finished (ROWID tId, unsigned b, unsigned lane);
void db_reaction_finish (reaction *r, ROWID result_tId, int offX, int offY, int gen, db_reaction_type type);
void db_reaction_emits (ROWID rId);
void db_store_emit (ROWID rId, ROWID oId, int offX, int offY, int gen);
