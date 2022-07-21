#include "SDL_error.h"
#include "SDL_error_c.h"
#include "SDL_log.h"

int
SDL_SetError(const char *fmt, ...)
{
    if (fmt != NULL) {
        va_list ap;
        SDL_error *error = SDL_GetErrBuf();

        error->error = 1;  /* mark error as valid */

        va_start(ap, fmt);
        SDL_vsnprintf(error->str, ERR_MAX_STRLEN, fmt, ap);
        va_end(ap);

        if (SDL_LogGetPriority(SDL_LOG_CATEGORY_ERROR) <= SDL_LOG_PRIORITY_DEBUG) {
            SDL_LogDebug(SDL_LOG_CATEGORY_ERROR, "%s", error->str);
        }
    }

    return -1;
}

const char *
SDL_GetError(void)
{
    const SDL_error *error = SDL_GetErrBuf();
    return error->error ? error->str : "";
}

char *
SDL_GetErrorMsg(char *errstr, int maxlen)
{
    const SDL_error *error = SDL_GetErrBuf();

    if (error->error) {
        SDL_strlcpy(errstr, error->str, maxlen);
    } else {
        *errstr = '\0';
    }

    return errstr;
}

void
SDL_ClearError(void)
{
    SDL_GetErrBuf()->error = 0;
}

int
SDL_Error(SDL_errorcode code)
{
    switch (code) {
    case SDL_ENOMEM:
        return SDL_SetError("Out of memory");
    case SDL_EFREAD:
        return SDL_SetError("Error reading from datastream");
    case SDL_EFWRITE:
        return SDL_SetError("Error writing to datastream");
    case SDL_EFSEEK:
        return SDL_SetError("Error seeking in datastream");
    case SDL_UNSUPPORTED:
        return SDL_SetError("That operation is not supported");
    default:
        return SDL_SetError("Unknown SDL error");
    }
}
