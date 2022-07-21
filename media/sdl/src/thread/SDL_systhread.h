#ifndef SDL_systhread_h_
#define SDL_systhread_h_
#include "SDL_thread.h"
#include "SDL_thread_c.h"

extern int SDL_SYS_CreateThread(SDL_Thread * thread);

extern void SDL_SYS_SetupThread(const char *name);

extern int SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority);

extern void SDL_SYS_WaitThread(SDL_Thread * thread);

extern void SDL_SYS_DetachThread(SDL_Thread * thread);

#endif /*SDL_systhread_h_*/