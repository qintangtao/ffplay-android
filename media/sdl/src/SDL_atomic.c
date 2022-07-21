#include "SDL_atomic.h"
#include "SDL_mutex.h"
#include "SDL_timer.h"

/* "REP NOP" is PAUSE, coded for tools that don't know it by that name. */
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    #define PAUSE_INSTRUCTION() __asm__ __volatile__("pause\n")  /* Some assemblers can't do REP NOP, so go with PAUSE. */
#elif (defined(__arm__) && __ARM_ARCH__ >= 7) || defined(__aarch64__)
    #define PAUSE_INSTRUCTION() __asm__ __volatile__("yield" ::: "memory")
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #define PAUSE_INSTRUCTION() _mm_pause()  /* this is actually "rep nop" and not a SIMD instruction. No inline asm in MSVC x86-64! */
#elif defined(__WATCOMC__) && defined(__386__)
    /* watcom assembler rejects PAUSE if CPU < i686, and it refuses REP NOP as an invalid combination. Hardcode the bytes.  */
    extern _inline void PAUSE_INSTRUCTION(void);
    #pragma aux PAUSE_INSTRUCTION = "db 0f3h,90h"
#else
    #define PAUSE_INSTRUCTION()
#endif

SDL_bool
SDL_AtomicTryLock(SDL_SpinLock *lock)
{
    return (__sync_lock_test_and_set(lock, 1) == 0);
}

void
SDL_AtomicLock(SDL_SpinLock *lock)
{
    int iterations = 0;
    while (!SDL_AtomicTryLock(lock)) {
        if (iterations < 32) {
            iterations++;
            PAUSE_INSTRUCTION();
        } else {
            SDL_Delay(0);
        }
    }
}

void
SDL_AtomicUnlock(SDL_SpinLock *lock)
{
    __sync_lock_release(lock);
}

static SDL_SpinLock locks[32];

static inline void
enterLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);
    SDL_AtomicLock(&locks[index]);
}

static inline void
leaveLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);
    SDL_AtomicUnlock(&locks[index]);
}


SDL_bool
SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval)
{
    return (SDL_bool) __sync_bool_compare_and_swap(&a->value, oldval, newval);
}
int
SDL_AtomicSet(SDL_atomic_t *a, int v)
{
    return __sync_lock_test_and_set(&a->value, v);
}
int
SDL_AtomicGet(SDL_atomic_t *a)
{
    return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST);
}
int
SDL_AtomicAdd(SDL_atomic_t *a, int v)
{
    return __sync_fetch_and_add(&a->value, v);
}