/* Database driver - replace mysql/mariadb with your DB, if you like */

void db_init (void);

bullet *db_bullet_load (const char *name);
object *db_load_space_ships (void);

typedef enum {dbrt_undef, dbrt_dies, dbrt_flyby, dbrt_stable, dbrt_unfinished} db_reaction_type;

void db_targets_keep (target **tgts, int nph);
bool db_reaction_keep (reaction *r);
bool db_is_reaction_finished (reaction *r);
void db_reaction_finish (reaction *r, ROWID result_tId, int offX, int offY, int gen, db_reaction_type type);
void db_store_emit (ROWID rId, ROWID oId, int offX, int offY, int gen);
