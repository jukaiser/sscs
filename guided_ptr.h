/* Wrapper for void * to keep track of # references so we know when free () would be OK */

#include <stdbool.h>


typedef struct
  {
    unsigned long ref_count;
    void *ptr;
  } guided;


guided *guide_alloc (void *p);
void guide_link (guided *gp);
bool guide_unlink (guided *gp);
void guide_dealloc (guided *gp);
