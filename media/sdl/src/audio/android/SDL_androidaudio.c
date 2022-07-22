#include "SDL_androidaudio.h"
#include "SDL_androidaudio_jni.h"
#include "SDL_stdinc.h"
#include "SDL_log.h"
#include "SDL_error.h"
#include "SDL_assert.h"
#include "../SDL_audio_c.h"

static SDL_AudioDevice* audioDevice = NULL;
static SDL_AudioDevice* captureDevice = NULL;


static int
ANDROIDAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format;


    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice 1");

    //SDL_assert((captureDevice == NULL) || !iscapture);
    //SDL_assert((audioDevice == NULL) || iscapture);

    if (iscapture) {
        captureDevice = this;
    } else {
        audioDevice = this;
    }

    this->hidden = (struct SDL_PrivateAudioData *) SDL_calloc(1, (sizeof *this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice 2");
    this->hidden->audioTrack = 0;
    this->hidden->audioBufferFormat = 0;
    this->hidden->audioBuffer = NULL;
    this->hidden->audioBufferPinned = NULL;
    this->hidden->captureBufferFormat = 0;
    this->hidden->captureBuffer = NULL;

    test_format = SDL_FirstAudioFormat(this->spec.format);
    while (test_format != 0) {
        if ((test_format == AUDIO_U8) ||
			(test_format == AUDIO_S16) ||
			(test_format == AUDIO_F32)) {
            this->spec.format = test_format;
            break;
        }
        test_format = SDL_NextAudioFormat();
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice 3");
    if (test_format == 0) {
        return SDL_SetError("No compatible audio format!");
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice 4");
    if (Android_JNI_OpenAudioDevice(iscapture, &this->spec, this->hidden) < 0) {
        return -1;
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice 5");
    SDL_CalculateAudioSpec(&this->spec);

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice okay");
    return 0;
}

static void
ANDROIDAUDIO_PlayDevice(_THIS)
{
    Android_JNI_WriteAudioBuffer(this->hidden);

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_PlayDevice okay");
}

static Uint8 *
ANDROIDAUDIO_GetDeviceBuf(_THIS)
{
    return Android_JNI_GetAudioBuffer(this->hidden);
}

static void
ANDROIDAUDIO_CloseDevice(_THIS)
{
    Android_JNI_CloseAudioDevice(this->hidden);
    if (this->iscapture) {
        SDL_assert(captureDevice == this);
        captureDevice = NULL;
    } else {
        SDL_assert(audioDevice == this);
        audioDevice = NULL;
    }
    SDL_free(this->hidden);

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_CloseDevice okay");
}

static int
ANDROIDAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = ANDROIDAUDIO_OpenDevice;
    impl->PlayDevice = ANDROIDAUDIO_PlayDevice;
    impl->GetDeviceBuf = ANDROIDAUDIO_GetDeviceBuf;
    impl->CloseDevice = ANDROIDAUDIO_CloseDevice;
    //impl->CaptureFromDevice = ANDROIDAUDIO_CaptureFromDevice;
    //impl->FlushCapture = ANDROIDAUDIO_FlushCapture;

    /* and the capabilities */
    //impl->HasCaptureSupport = SDL_TRUE;
    impl->HasCaptureSupport = SDL_FALSE;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultCaptureDevice = 1;


    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_Init okay");
    return 1;   /* this audio target is available. */
}

AudioBootStrap ANDROIDAUDIO_bootstrap = {
    "android", "SDL Android audio driver", ANDROIDAUDIO_Init, 0
};