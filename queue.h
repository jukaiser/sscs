/* Maintaining a weighted queue of reactions to handle */

void queue_init (void);
bool queue_insert (int cost, ROWID tId, uint8_t phase, uint8_t  state, uint8_t  b, uint8_t  lane);
reaction *queue_grabfront (void);
void queue_info (void);
void queue_save_state (const char *fname);
void queue_restore_state (const char *fname);
void queue_extend_depth (int delta);
