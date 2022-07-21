#ifndef SDL_opensles_audio_h_
#define SDL_opensles_audio_h_

#include "../SDL_sysaudio.h"
#include "SDL_mutex.h"

#define _THIS   SDL_AudioDevice *this

#define NUM_BUFFERS 2           /* -- Don't lower this! */

struct SDL_PrivateAudioData
{
    Uint8   *mixbuff;
    int      next_buffer;
    Uint8   *pmixbuff[NUM_BUFFERS];
    SDL_sem *playsem;
};


#endif /*SDL_opensles_audio_h_*/