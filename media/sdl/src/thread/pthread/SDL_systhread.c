#include "../SDL_systhread.h"
#include "SDL_error.h"


#ifdef __ANDROID__
#include "../../core/android/SDL_android.h"
#endif

static void *
RunThread(void *data)
{
#ifdef __ANDROID__
    Android_JNI_SetupThread();
#endif
    SDL_RunThread((SDL_Thread *) data);
    return NULL;
}

int
SDL_SYS_CreateThread(SDL_Thread * thread)
{
    pthread_attr_t type;

    if (pthread_attr_init(&type) != 0) {
        return SDL_SetError("Couldn't initialize pthread attributes");
    }
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

    if (thread->stacksize) {
        pthread_attr_setstacksize(&type, thread->stacksize);
    }

    if (pthread_create(&thread->handle, &type, RunThread, thread) != 0) {
        return SDL_SetError("Not enough resources to create thread");
    }

    return 0;
 }

void
SDL_SYS_SetupThread(const char *name)
{
    pthread_setname_np(pthread_self(), name);
}

int
SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    struct sched_param sched;
    int policy;
    pthread_t thread = pthread_self();

    if (pthread_getschedparam(thread, &policy, &sched) != 0) {
        return SDL_SetError("pthread_getschedparam() failed");
        return -1;
    }

    if (priority == SDL_THREAD_PRIORITY_LOW) {
        sched.sched_priority = sched_get_priority_min(policy);
    } else if (priority == SDL_THREAD_PRIORITY_TIME_CRITICAL) {
        sched.sched_priority = sched_get_priority_max(policy);
    } else {
        int min_priority = sched_get_priority_min(policy);
        int max_priority = sched_get_priority_max(policy);
        sched.sched_priority = (min_priority + (max_priority - min_priority) / 2);
    }

    if (pthread_setschedparam(thread, policy, &sched) != 0) {
       return SDL_SetError("pthread_setschedparam() failed");
       return -1;
    }

    return 0;
}

void SDL_SYS_WaitThread(SDL_Thread * thread)
{
    pthread_join(thread->handle, 0);
}

void SDL_SYS_DetachThread(SDL_Thread * thread)
{
    pthread_detach(thread->handle);
}

SDL_threadID
SDL_ThreadID(void)
{
    return ((SDL_threadID) pthread_self());
}