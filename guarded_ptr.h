/* Wrapper for void * to keep track of # references so we know when free () would be OK */

typedef struct
  {
    unsigned long ref_count;
    void *ptr;
  } guarded;


guarded *guard_alloc (void *p);
void guard_link (guarded *gp);
bool guard_unlink (guarded *gp);
void guard_dealloc (guarded *gp);
