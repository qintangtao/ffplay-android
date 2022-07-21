#ifndef SDL_androidaudio_jni_h_
#define SDL_androidaudio_jni_h_

#include "SDL_androidaudio.h"

int Android_JNI_OpenAudioDevice(int iscapture, SDL_AudioSpec *spec, struct SDL_PrivateAudioData *data);

void * Android_JNI_GetAudioBuffer(struct SDL_PrivateAudioData *data);

void Android_JNI_WriteAudioBuffer(struct SDL_PrivateAudioData *data);

void Android_JNI_CloseAudioDevice(struct SDL_PrivateAudioData *data);


#endif /*SDL_androidaudio_jni_h_*/