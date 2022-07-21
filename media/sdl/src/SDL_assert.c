#include "SDL_assert.h"
#include "SDL_atomic.h"
#include "SDL_mutex.h"
#include "SDL_log.h"

static SDL_AssertState SDLCALL
SDL_PromptAssertion(const SDL_AssertData *data, void *userdata);

static SDL_mutex *assertion_mutex = NULL;

static SDL_AssertionHandler assertion_handler = SDL_PromptAssertion;
static void *assertion_userdata = NULL;


SDL_AssertState
SDL_ReportAssertion(SDL_AssertData *data, const char *func, const char *file,
                    int line)
{
    SDL_AssertState state = SDL_ASSERTION_IGNORE;

    static SDL_SpinLock spinlock = 0;
    SDL_AtomicLock(&spinlock);
    if (assertion_mutex == NULL) { /* never called SDL_Init()? */
        assertion_mutex = SDL_CreateMutex();
        if (assertion_mutex == NULL) {
            SDL_AtomicUnlock(&spinlock);
            return SDL_ASSERTION_IGNORE;   /* oh well, I guess. */
        }
    }
    SDL_AtomicUnlock(&spinlock);

    if (SDL_LockMutex(assertion_mutex) < 0) {
        return SDL_ASSERTION_IGNORE;   /* oh well, I guess. */
    }

    /* doing this because Visual C is upset over assigning in the macro. */
    if (data->trigger_count == 0) {
        data->function = func;
        data->filename = file;
        data->linenum = line;
    }

    if (!data->always_ignore) {
        state = assertion_handler(data, assertion_userdata);
    } else {
        state = SDL_ASSERTION_BREAK;
    }

    SDL_UnlockMutex(assertion_mutex);

    return state;
}

void SDL_AssertionsQuit(void)
{
    if (assertion_mutex != NULL) {
        SDL_DestroyMutex(assertion_mutex);
        assertion_mutex = NULL;
    }
}

static void
debug_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_WARN, fmt, ap);
    va_end(ap);
}

static SDL_AssertState SDLCALL
SDL_PromptAssertion(const SDL_AssertData *data, void *userdata)
{
#ifdef __WIN32__
    #define ENDLINE "\r\n"
#else
    #define ENDLINE "\n"
#endif

    char *message;

    (void) userdata;  /* unused in default handler. */

    message = SDL_stack_alloc(char, SDL_MAX_LOG_MESSAGE);
    if (!message) {
        return SDL_ASSERTION_ABORT;
    }

    SDL_snprintf(message, SDL_MAX_LOG_MESSAGE,
                 "Assertion failure at %s (%s:%d), triggered %u %s:" ENDLINE
                    "  '%s'",
                 data->function, data->filename, data->linenum,
                 data->trigger_count, (data->trigger_count == 1) ? "time" : "times",
                 data->condition);

    debug_print("\n\n%s\n\n", message);

    SDL_stack_free(message);

    return SDL_ASSERTION_BREAK;
}
