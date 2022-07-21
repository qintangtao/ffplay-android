#ifndef SDL_systls_h_
#define SDL_systls_h_

extern SDL_TLSData *SDL_SYS_GetTLSData(void);

extern int SDL_SYS_SetTLSData(SDL_TLSData *data);

#endif /*SDL_systls_h_*/