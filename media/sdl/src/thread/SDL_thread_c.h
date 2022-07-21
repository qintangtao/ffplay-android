#ifndef SDL_thread_c_h_
#define SDL_thread_c_h_

#include "SDL_thread.h"
#include "SDL_atomic.h"

#include "pthread/SDL_systhread_c.h"

typedef enum SDL_ThreadState
{
    SDL_THREAD_STATE_ALIVE,
    SDL_THREAD_STATE_DETACHED,
    SDL_THREAD_STATE_ZOMBIE,
    SDL_THREAD_STATE_CLEANED,
} SDL_ThreadState;

/* This is the system-independent thread info structure */
struct SDL_Thread
{
    SDL_threadID threadid;
    SYS_ThreadHandle handle;
    SDL_atomic_t state;
    int status;
    int stacksize;
    char *name;
    int (* userfunc) (void *);
    void *userdata;
};

extern void SDL_RunThread(SDL_Thread *thread);

extern SDL_Thread *SDLCALL
SDL_CreateThreadInternal(SDL_ThreadFunction fn, const char *name,
                         const size_t stacksize, void *data);


#endif /*SDL_thread_c_h_*/