/* Maintaining a weighted queue */

#include <stdbool.h>

void queue_init (void);
bool queue_insert (int cost, void *val);
void *queue_grabfront (void);
