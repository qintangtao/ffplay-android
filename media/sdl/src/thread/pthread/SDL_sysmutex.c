#include "SDL_mutex.h"
#include "SDL_stdinc.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

struct SDL_mutex
{
    pthread_mutex_t id;
};

struct SDL_semaphore
{
    sem_t sem;
};

struct SDL_cond
{
    pthread_cond_t cond;
};


SDL_mutex *
SDL_CreateMutex(void)
{
    SDL_mutex *mutex;

    /* Allocate the structure */
    mutex = (SDL_mutex *) SDL_calloc(1, sizeof(*mutex));
    if (mutex) {
        if (pthread_mutex_init(&mutex->id, NULL) != 0) {
            SDL_free(mutex);
            mutex = NULL;
        }
    }

    return (mutex);
}

void
SDL_DestroyMutex(SDL_mutex * mutex)
{
    if (mutex) {
        pthread_mutex_destroy(&mutex->id);
        SDL_free(mutex);
    }
}

int
SDL_LockMutex(SDL_mutex * mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    return pthread_mutex_lock(&mutex->id);
}

int
SDL_TryLockMutex(SDL_mutex * mutex)
{
    int retval;
    int result;
    if (mutex == NULL) {
        return -1;
    }

    retval = 0;
    result = pthread_mutex_trylock(&mutex->id);
    if (result != 0) {
        if (result == EBUSY) {
            retval = SDL_MUTEX_TIMEDOUT;
        } else {
            retval = -1;
        }
    }
    return retval;
}

int
SDL_UnlockMutex(SDL_mutex * mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    return pthread_mutex_unlock(&mutex->id);
}


SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem *sem = (SDL_sem *) SDL_malloc(sizeof(SDL_sem));
    if (sem) {
        if (sem_init(&sem->sem, 0, initial_value) < 0) {
            SDL_free(sem);
            sem = NULL;
        }
    }
    return sem;
}

void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
        sem_destroy(&sem->sem);
        SDL_free(sem);
    }
}


int
SDL_SemTryWait(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }
    retval = SDL_MUTEX_TIMEDOUT;
    if (sem_trywait(&sem->sem) == 0) {
        retval = 0;
    }
    return retval;
}


int
SDL_SemWait(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }

    do {
        retval = sem_wait(&sem->sem);
    } while (retval < 0 && errno == EINTR);

    if (retval < 0) {
        retval = -1;
    }
    return retval;
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    int retval;
    struct timeval now;
    struct timespec ts_timeout;

    if (!sem) {
        return -1;
    }

    /* Try the easy cases first */
    if (timeout == 0) {
        return SDL_SemTryWait(sem);
    }
    if (timeout == SDL_MUTEX_MAXWAIT) {
        return SDL_SemWait(sem);
    }

    gettimeofday(&now, NULL);

    /* Add our timeout to current time */
    ts_timeout.tv_sec = now.tv_sec + (timeout / 1000);
    ts_timeout.tv_nsec = (now.tv_usec + (timeout % 1000) * 1000) * 1000;

    /* Wrap the second if needed */
    if (ts_timeout.tv_nsec > 1000000000) {
        ts_timeout.tv_sec += 1;
        ts_timeout.tv_nsec -= 1000000000;
    }

    /* Wait. */
    do {
        retval = sem_timedwait(&sem->sem, &ts_timeout);
    } while (retval < 0 && errno == EINTR);

    if (retval < 0) {
        if (errno == ETIMEDOUT) {
            retval = SDL_MUTEX_TIMEDOUT;
        }
    }

    return retval;
}

Uint32
SDL_SemValue(SDL_sem * sem)
{
    int ret = 0;
    if (sem) {
        sem_getvalue(&sem->sem, &ret);
        if (ret < 0) {
            ret = 0;
        }
    }
    return (Uint32) ret;
}

int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        return -1;
    }

    return sem_post(&sem->sem);
}


SDL_cond *
SDL_CreateCond(void)
{
    SDL_cond *cond;

    cond = (SDL_cond *) SDL_malloc(sizeof(SDL_cond));
    if (cond) {
        if (pthread_cond_init(&cond->cond, NULL) != 0) {
            SDL_free(cond);
            cond = NULL;
        }
    }
    return (cond);
}

void
SDL_DestroyCond(SDL_cond * cond)
{
    if (cond) {
        pthread_cond_destroy(&cond->cond);
        SDL_free(cond);
    }
}

int
SDL_CondSignal(SDL_cond * cond)
{
    if (!cond) {
        return -1;
    }

    return pthread_cond_signal(&cond->cond);
}

/* Restart all threads that are waiting on the condition variable */
int
SDL_CondBroadcast(SDL_cond * cond)
{
    if (!cond) {
        return -1;
    }

    return pthread_cond_broadcast(&cond->cond);
}

int
SDL_CondWaitTimeout(SDL_cond * cond, SDL_mutex * mutex, Uint32 ms)
{
    int retval;
    struct timeval delta;
    struct timespec abstime;

    if (!cond || mutex) {
        return -1;
    }

    gettimeofday(&delta, NULL);

    abstime.tv_sec = delta.tv_sec + (ms / 1000);
    abstime.tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
    if (abstime.tv_nsec > 1000000000) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }

  tryagain:
    retval = pthread_cond_timedwait(&cond->cond, &mutex->id, &abstime);
    switch (retval) {
    case EINTR:
        goto tryagain;
        /* break; -Wunreachable-code-break */
    case ETIMEDOUT:
        retval = SDL_MUTEX_TIMEDOUT;
        break;
    case 0:
        break;
    default:
        retval = -1;
    }
    return retval;
}

int
SDL_CondWait(SDL_cond * cond, SDL_mutex * mutex)
{
    if (!cond || !mutex) {
        return -1;
    }

    return pthread_cond_wait(&cond->cond, &mutex->id);
}