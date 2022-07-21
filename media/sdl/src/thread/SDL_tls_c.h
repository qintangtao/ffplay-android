#ifndef SDL_tls_c_h_
#define SDL_tls_c_h_

#include "SDL_config.h"

typedef struct {
    unsigned int limit;
    struct {
        void *data;
        void (SDLCALL *destructor)(void*);
    } array[1];
} SDL_TLSData;

#define TLS_ALLOC_CHUNKSIZE 4

extern SDL_TLSData *SDL_Generic_GetTLSData(void);

extern int SDL_Generic_SetTLSData(SDL_TLSData *data);

extern void SDL_TLSCleanup();


#endif /*SDL_tls_c_h_*/