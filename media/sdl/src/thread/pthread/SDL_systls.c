#include "SDL_atomic.h"
#include "../SDL_tls_c.h"

#include <pthread.h>

#define INVALID_PTHREAD_KEY ((pthread_key_t)-1)

static pthread_key_t thread_local_storage = INVALID_PTHREAD_KEY;
static SDL_bool generic_local_storage = SDL_FALSE;

SDL_TLSData *
SDL_SYS_GetTLSData(void)
{
    if (thread_local_storage == INVALID_PTHREAD_KEY && !generic_local_storage) {
        static SDL_SpinLock lock;
        SDL_AtomicLock(&lock);
        if (thread_local_storage == INVALID_PTHREAD_KEY && !generic_local_storage) {
            pthread_key_t storage;
            if (pthread_key_create(&storage, NULL) == 0) {
                SDL_MemoryBarrierRelease();
                thread_local_storage = storage;
            } else {
                generic_local_storage = SDL_TRUE;
            }
        }
        SDL_AtomicUnlock(&lock);
    }
    if (generic_local_storage) {
        return SDL_Generic_GetTLSData();
    }
    SDL_MemoryBarrierAcquire();
    return (SDL_TLSData *)pthread_getspecific(thread_local_storage);
}

int
SDL_SYS_SetTLSData(SDL_TLSData *data)
{
    if (generic_local_storage) {
        return SDL_Generic_SetTLSData(data);
    }
    if (pthread_setspecific(thread_local_storage, data) != 0) {
        return SDL_SetError("pthread_setspecific() failed");
    }
    return 0;
}