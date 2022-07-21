#include "SDL_thread.h"
#include "SDL_assert.h"
#include "SDL_systhread.h"
#include "SDL_tls_c.h"
#include "SDL_thread_c.h"

void
SDL_RunThread(SDL_Thread *thread)
{
    void *userdata = thread->userdata;
    int (SDLCALL * userfunc) (void *) = thread->userfunc;

    int *statusloc = &thread->status;

    SDL_SYS_SetupThread(thread->name);

    thread->threadid = SDL_ThreadID();

    *statusloc = userfunc(userdata);

    SDL_TLSCleanup();

    if (!SDL_AtomicCAS(&thread->state, SDL_THREAD_STATE_ALIVE, SDL_THREAD_STATE_ZOMBIE)) {
        if (SDL_AtomicCAS(&thread->state, SDL_THREAD_STATE_DETACHED, SDL_THREAD_STATE_CLEANED)) {
            if (thread->name) {
                SDL_free(thread->name);
            }
            SDL_free(thread);
        }
    }
}

SDL_Thread *SDLCALL
SDL_CreateThreadInternal(SDL_ThreadFunction fn, const char *name,
                         const size_t stacksize, void *data)
 {
    SDL_Thread *thread;
    int ret;

    thread = (SDL_Thread *) SDL_calloc(1, sizeof(SDL_Thread));
    if (thread == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    thread->status = -1;
    SDL_AtomicSet(&thread->state, SDL_THREAD_STATE_ALIVE);

    if (name != NULL) {
        thread->name = SDL_strdup(name);
        if (thread->name == NULL) {
            SDL_OutOfMemory();
            SDL_free(thread);
            return NULL;
        }
    }

    thread->userfunc = fn;
    thread->userdata = data;
    thread->stacksize = stacksize;

    ret = SDL_SYS_CreateThread(thread);
    if (ret < 0) {
        SDL_free(thread->name);
        SDL_free(thread);
        thread = NULL;
    }

    return thread;
 }

SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data)
{
    return SDL_CreateThreadInternal(fn, name, 0, data);
}

const char *
SDL_GetThreadName(SDL_Thread * thread)
{
    if (thread) {
        return thread->name;
    } else {
        return NULL;
    }
}

SDL_threadID
SDL_GetThreadID(SDL_Thread * thread)
{
    SDL_threadID id;

    if (thread) {
        id = thread->threadid;
    } else {
        id = SDL_ThreadID();
    }
    return id;
}

void
SDL_WaitThread(SDL_Thread * thread, int *status)
{
    if (thread) {
        SDL_SYS_WaitThread(thread);
        if (status) {
            *status = thread->status;
        }
        if (thread->name) {
            SDL_free(thread->name);
        }
        SDL_free(thread);
    }
}


void
SDL_DetachThread(SDL_Thread * thread)
{
    if (!thread) {
        return;
    }

    if (SDL_AtomicCAS(&thread->state, SDL_THREAD_STATE_ALIVE, SDL_THREAD_STATE_DETACHED)) {
        SDL_SYS_DetachThread(thread);
    } else {
        const int thread_state = SDL_AtomicGet(&thread->state);
        if ((thread_state == SDL_THREAD_STATE_DETACHED) || (thread_state == SDL_THREAD_STATE_CLEANED)) {
            return;
        } else if (thread_state == SDL_THREAD_STATE_ZOMBIE) {
            SDL_WaitThread(thread, NULL);
        } else {
            SDL_assert(0 && "Unexpected thread state");
        }
    }
}

int
SDL_SetThreadPriority(SDL_ThreadPriority priority)
{
    return SDL_SYS_SetThreadPriority(priority);
}