#ifdef NO_PROFILING
#define	_prof_init()
#define _prof_enter()
#define _prof_leave(x)
#else
void _prof_init ();
void _prof_enter ();
void _prof_leave (const char *msg);
#endif
