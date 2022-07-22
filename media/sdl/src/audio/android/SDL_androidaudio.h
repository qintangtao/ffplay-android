#ifndef SDL_androidaudio_h_
#define SDL_androidaudio_h_

#include "../SDL_sysaudio.h"
#include "SDL_mutex.h"
#include "SDL_audiotrack.h"
#include "../../core/android/SDL_android.h"

#define _THIS   SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    SDL_Android_AudioTrack *audioTrack;

    int audioBufferFormat;
    jobject audioBuffer;
    void *audioBufferPinned;

    int captureBufferFormat;
    jobject captureBuffer;
};


#endif /*SDL_androidaudio_h_*/