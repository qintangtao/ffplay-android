#ifndef SDL_error_c_h_
#define SDL_error_c_h_

#define ERR_MAX_STRLEN  128

typedef struct SDL_error
{
    int error; /* This is a numeric value corresponding to the current error */
    char str[ERR_MAX_STRLEN];
} SDL_error;

/* Defined in SDL_thread.c */
extern SDL_error *SDL_GetErrBuf(void);

#endif /*SDL_error_c_h_*/