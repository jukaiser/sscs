/* Maintaining a weighted queue of reactions to handle */

void queue_init (void);
bool queue_insert (int cost, ROWID tId, uint8_t  b, uint8_t  lane, uint8_t  delta);
reaction *queue_grabfront (void);
void queue_info (void);
