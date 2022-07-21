#include "SDL_thread.h"
#include "SDL_mutex.h"
#include "SDL_atomic.h"
#include "SDL_thread_c.h"
#include "SDL_tls_c.h"
#include "SDL_systls.h"
#include "../SDL_error_c.h"

#include <pthread.h>

typedef struct SDL_TLSEntry {
    SDL_threadID thread;
    SDL_TLSData *storage;
    struct SDL_TLSEntry *next;
} SDL_TLSEntry;

static SDL_mutex *SDL_generic_TLS_mutex;
static SDL_TLSEntry *SDL_generic_TLS;

SDL_TLSData *
SDL_Generic_GetTLSData(void)
{
    SDL_threadID thread = SDL_ThreadID();
    SDL_TLSEntry *entry;
    SDL_TLSData *storage = NULL;

    if (!SDL_generic_TLS_mutex) {
        static SDL_SpinLock tls_lock;
        SDL_AtomicLock(&tls_lock);
        if (!SDL_generic_TLS_mutex) {
            SDL_mutex *mutex = SDL_CreateMutex();
            SDL_MemoryBarrierRelease();
            SDL_generic_TLS_mutex = mutex;
            if (!SDL_generic_TLS_mutex) {
                SDL_AtomicUnlock(&tls_lock);
                return NULL;
            }
        }
        SDL_AtomicUnlock(&tls_lock);
    }

    SDL_MemoryBarrierAcquire();
    SDL_LockMutex(SDL_generic_TLS_mutex);
    for (entry = SDL_generic_TLS; entry; entry = entry->next) {
        if (entry->thread == thread) {
            storage = entry->storage;
            break;
        }
    }

    SDL_UnlockMutex(SDL_generic_TLS_mutex);

    return storage;
}

int
SDL_Generic_SetTLSData(SDL_TLSData *storage)
{
    SDL_threadID thread = SDL_ThreadID();
    SDL_TLSEntry *prev, *entry;

    SDL_LockMutex(SDL_generic_TLS_mutex);
    prev = NULL;
    for (entry = SDL_generic_TLS; entry; entry = entry->next) {
        if (entry->thread == thread) {
            if (storage) {
                entry->storage = storage;
            } else {
                if (prev) {
                    prev->next = entry->next;
                } else {
                    SDL_generic_TLS = entry->next;
                }
                SDL_free(entry);
            }
            break;
        }
        prev = entry;
    }
    if (!entry) {
        entry = (SDL_TLSEntry *)SDL_malloc(sizeof(*entry));
        if (entry) {
            entry->thread = thread;
            entry->storage = storage;
            entry->next = SDL_generic_TLS;
            SDL_generic_TLS = entry;
        }
    }
    SDL_UnlockMutex(SDL_generic_TLS_mutex);

    if (!entry) {
        return SDL_OutOfMemory();
    }
    return 0;
}

SDL_TLSID
SDL_TLSCreate()
{
    static SDL_atomic_t SDL_tls_id;
    return SDL_AtomicIncRef(&SDL_tls_id)+1;
}

void *
SDL_TLSGet(SDL_TLSID id)
{
    SDL_TLSData *storage;

    storage = SDL_SYS_GetTLSData();
    if (!storage || id == 0 || id > storage->limit) {
        return NULL;
    }
    return storage->array[id-1].data;
}

int
SDL_TLSSet(SDL_TLSID id, const void *value, void (SDLCALL *destructor)(void *))
{
    SDL_TLSData *storage;

    if (id == 0) {
        return SDL_InvalidParamError("id");
    }

    storage = SDL_SYS_GetTLSData();
    if (!storage || (id > storage->limit)) {
        unsigned int i, oldlimit, newlimit;

        oldlimit = storage ? storage->limit : 0;
        newlimit = (id + TLS_ALLOC_CHUNKSIZE);
        storage = (SDL_TLSData *)SDL_realloc(storage, sizeof(*storage)+(newlimit-1)*sizeof(storage->array[0]));
        if (!storage) {
            return SDL_OutOfMemory();
        }
        storage->limit = newlimit;
        for (i = oldlimit; i < newlimit; ++i) {
            storage->array[i].data = NULL;
            storage->array[i].destructor = NULL;
        }
        if (SDL_SYS_SetTLSData(storage) != 0) {
            return -1;
        }
    }

    storage->array[id-1].data = SDL_const_cast(void*, value);
    storage->array[id-1].destructor = destructor;
    return 0;
}


void
SDL_TLSCleanup()
{
    SDL_TLSData *storage;

    storage = SDL_SYS_GetTLSData();
    if (storage) {
        unsigned int i;
        for (i = 0; i < storage->limit; ++i) {
            if (storage->array[i].destructor) {
                storage->array[i].destructor(storage->array[i].data);
            }
        }
        SDL_SYS_SetTLSData(NULL);
        SDL_free(storage);
    }
}


SDL_error *
SDL_GetErrBuf(void)
{
    static SDL_SpinLock tls_lock;
    static SDL_bool tls_being_created;
    static SDL_TLSID tls_errbuf;
    static SDL_error SDL_global_errbuf;
    const SDL_error *ALLOCATION_IN_PROGRESS = (SDL_error *)-1;
    SDL_error *errbuf;

    if (!tls_errbuf && !tls_being_created) {
        SDL_AtomicLock(&tls_lock);
        if (!tls_errbuf) {
            SDL_TLSID slot;
            tls_being_created = SDL_TRUE;
            slot = SDL_TLSCreate();
            tls_being_created = SDL_FALSE;
            SDL_MemoryBarrierRelease();
            tls_errbuf = slot;
        }
        SDL_AtomicUnlock(&tls_lock);
    }
    if (!tls_errbuf) {
        return &SDL_global_errbuf;
    }

    SDL_MemoryBarrierAcquire();
    errbuf = (SDL_error *)SDL_TLSGet(tls_errbuf);
    if (errbuf == ALLOCATION_IN_PROGRESS) {
        return &SDL_global_errbuf;
    }
    if (!errbuf) {
        SDL_TLSSet(tls_errbuf, ALLOCATION_IN_PROGRESS, NULL);
        errbuf = (SDL_error *)SDL_malloc(sizeof(*errbuf));
        if (!errbuf) {
            SDL_TLSSet(tls_errbuf, NULL, NULL);
            return &SDL_global_errbuf;
        }
        SDL_zerop(errbuf);
        SDL_TLSSet(tls_errbuf, errbuf, SDL_free);
    }
    return errbuf;
}