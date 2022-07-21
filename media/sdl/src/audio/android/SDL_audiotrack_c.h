#ifndef SDL_audio_track_c_h_
#define SDL_audio_track_c_h_

#include "../../core/android/SDL_android.h"

struct SDL_Android_AudioTrack{
    SDL_bool isCapture;
    jobject audioTrack;

    //out
    int sampleRate;
    int audioFormat;
    int channelCount;
    int desiredFrames;
} ;

#endif /*SDL_audio_track_c_h_*/