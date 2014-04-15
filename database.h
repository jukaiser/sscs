/* Database driver - replace mysql/mariadb with your DB, if you like */

/* low level */
#ifdef _mysql_h
MYSQL *con;
void finish_with_error(MYSQL *con);
#endif
/* low level */

void db_init (void);

object *db_load_space_ships (void);
bullet *db_load_bullets_for (const char *shipname);
part *db_load_parts_for (const char *shipname);

ROWID db_target_store (uint32_t hash, const char *rle, unsigned nph, unsigned width, unsigned height);
ROWID db_target_lookup (uint32_t hash, const char *rle);
void db_target_reload (target *tgt, ROWID tId, unsigned phase);
unsigned db_target_fetch (ROWID tId);
ROWID db_reaction_keep (reaction *r, result *res);
ROWID db_is_reaction_finished (ROWID tId, unsigned phase, unsigned b, unsigned lane, unsigned *cost);
void db_store_transition (ROWID rId, unsigned old_state, unsigned new_state, unsigned rephase, ROWID pId, unsigned total_cost, unsigned delta_cost);
void db_store_emit (ROWID rId, unsigned seq, ROWID oId, int offX, int offY, int gen);
