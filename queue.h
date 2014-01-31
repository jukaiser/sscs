/* Maintaining a weighted queue */

void queue_init (void);
bool queue_insert (int cost, void *val);
void *queue_grabfront (void);
