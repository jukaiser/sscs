/* Database driver - replace mysql/mariadb with your DB, if you like */

void db_init (void);

object *db_load_space_ships (void);
bullet *db_load_bullets_for (const char *shipname);
part *db_load_parts_for (const char *shipname);

void db_target_store (target *tgt, const char *rle, ROWID prev);
void db_target_link (ROWID curr, ROWID nxt);
bool db_target_lookup (target *tgt, const char *rle);
void db_target_fetch (reaction *r, target *t);
ROWID db_reaction_keep (reaction *r, result *res);
bool db_is_reaction_finished (ROWID tId, unsigned phase, unsigned b, unsigned lane);
void db_store_emit (ROWID rId, ROWID oId, int offX, int offY, int gen);
