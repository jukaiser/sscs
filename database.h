/* Database driver - replace mysql/mariadb with your DB, if you like */

void db_init (void);
bool db_target_exists (const char *rle, int sizeX, int sizeY);
void db_target_insert (const char *rle, int sizeX, int sizeY);

bullet *db_bullet_load (const char *name);
