#include "SDL_androidaudio.h"
#include "SDL_stdinc.h"
#include "SDL_log.h"
#include "SDL_error.h"
#include "SDL_assert.h"
#include "../SDL_audio_c.h"

static SDL_AudioDevice* audioDevice = NULL;
static SDL_AudioDevice* captureDevice = NULL;

static int
Android_OpenAudioDevice(int iscapture, SDL_AudioSpec *spec, struct SDL_PrivateAudioData *audiodata) {
   int audioformat;
   jobject jbufobj = NULL;
   jboolean isCopy;
   SDL_Android_AudioTrack *audiotrack;

   JNIEnv *env = Android_JNI_GetEnv();
   if (!env) {
       return SDL_SetError("Android_JNI_GetEnv null");
   }

   switch (spec->format) {
   case AUDIO_U8:
       audioformat = ENCODING_PCM_8BIT;
       break;
   case AUDIO_S16:
       audioformat = ENCODING_PCM_16BIT;
       break;
   case AUDIO_F32:
       audioformat = ENCODING_PCM_FLOAT;
       break;
   default:
       return SDL_SetError("Unsupported audio format: 0x%x", spec->format);
   }

   if (iscapture) {
       SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: opening device for capture");
       audiotrack = AudioTrack_Open(SDL_TRUE, spec->freq, audioformat, spec->channels, spec->samples);
   } else {
       SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: opening device for output");
       audiotrack = AudioTrack_Open(SDL_FALSE, spec->freq, audioformat, spec->channels, spec->samples);
   }
   if (!audiotrack) {
       return SDL_SetError("Java-side initialization failed");
   }

   audioformat = AudioTrack_audioFormat(audiotrack);
   switch(audioformat) {
   case ENCODING_PCM_8BIT:
       spec->format = AUDIO_U8;
       break;
   case ENCODING_PCM_16BIT:
       spec->format = AUDIO_S16;
       break;
   case ENCODING_PCM_FLOAT:
       spec->format = AUDIO_F32;
       break;
   default:
       AudioTrack_Close(audiotrack);
       return SDL_SetError("Unexpected audio format from Java: %d\n", audioformat);
   }

   spec->freq = AudioTrack_sampleRate(audiotrack);
   spec->channels = AudioTrack_channelCount(audiotrack);
   spec->samples = AudioTrack_desiredFrames(audiotrack);

   switch(audioformat) {
   case ENCODING_PCM_8BIT:
       {
           jbyteArray audioBufferLocal = (*env)->NewByteArray(env, spec->samples * spec->channels);
           if (audioBufferLocal) {
               jbufobj = (*env)->NewGlobalRef(env, audioBufferLocal);
               (*env)->DeleteLocalRef(env, audioBufferLocal);
           }
       }
       break;
   case ENCODING_PCM_16BIT:
       {
           jshortArray audioBufferLocal = (*env)->NewShortArray(env, spec->samples * spec->channels);
           if (audioBufferLocal) {
               jbufobj = (*env)->NewGlobalRef(env, audioBufferLocal);
               (*env)->DeleteLocalRef(env, audioBufferLocal);
           }
       }
       break;
   case ENCODING_PCM_FLOAT:
       {
           jfloatArray audioBufferLocal = (*env)->NewFloatArray(env, spec->samples * spec->channels);
           if (audioBufferLocal) {
               jbufobj = (*env)->NewGlobalRef(env, audioBufferLocal);
               (*env)->DeleteLocalRef(env, audioBufferLocal);
           }
       }
       break;
   }

   if (jbufobj == NULL) {
       SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: could not allocate an audio buffer");
       AudioTrack_Close(audiotrack);
       return SDL_OutOfMemory();
   }

   if (iscapture) {
       audiodata->captureBufferFormat = audioformat;
       audiodata->captureBuffer = jbufobj;
   } else {
       audiodata->audioBufferFormat = audioformat;
       audiodata->audioBuffer = jbufobj;
   }

   if (!iscapture) {
       isCopy = JNI_FALSE;
       switch (audioformat) {
       case ENCODING_PCM_8BIT:
           audiodata->audioBufferPinned = (*env)->GetByteArrayElements(env, (jbyteArray)audiodata->audioBuffer, &isCopy);
           break;
       case ENCODING_PCM_16BIT:
           audiodata->audioBufferPinned = (*env)->GetShortArrayElements(env, (jshortArray)audiodata->audioBuffer, &isCopy);
           break;
       case ENCODING_PCM_FLOAT:
           audiodata->audioBufferPinned = (*env)->GetFloatArrayElements(env, (jfloatArray)audiodata->audioBuffer, &isCopy);
           break;
       }
   }

   audiodata->audioTrack = audiotrack;

   return 0;
}

static int
ANDROIDAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format;

    SDL_assert((captureDevice == NULL) || !iscapture);
    SDL_assert((audioDevice == NULL) || iscapture);

    if (iscapture) {
        captureDevice = this;
    } else {
        audioDevice = this;
    }

    this->hidden = (struct SDL_PrivateAudioData *) SDL_calloc(1, (sizeof *this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }

    this->hidden->audioTrack = NULL;
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

    if (test_format == 0) {
        return SDL_SetError("No compatible audio format!");
    }

    if (Android_OpenAudioDevice(iscapture, &this->spec, this->hidden) < 0) {
        return -1;
    }

    SDL_CalculateAudioSpec(&this->spec);

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_OpenDevice okay");
    return 0;
}

static void
ANDROIDAUDIO_PlayDevice(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;

    JNIEnv *env = Android_JNI_GetEnv();

    switch (audiodata->audioBufferFormat) {
    case ENCODING_PCM_8BIT:
        (*env)->ReleaseByteArrayElements(env, (jbyteArray)audiodata->audioBuffer, (jbyte *)audiodata->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteByteBuffer(audiodata->audioTrack, (jbyteArray)audiodata->audioBuffer);
        break;
    case ENCODING_PCM_16BIT:
        (*env)->ReleaseShortArrayElements(env, (jshortArray)audiodata->audioBuffer, (jshort *)audiodata->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteShortBuffer(audiodata->audioTrack, (jshortArray)audiodata->audioBuffer);
        break;
    case ENCODING_PCM_FLOAT:
        (*env)->ReleaseFloatArrayElements(env, (jfloatArray)audiodata->audioBuffer, (jfloat *)audiodata->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteFloatBuffer(audiodata->audioTrack, (jfloatArray)audiodata->audioBuffer);
        break;
    default:
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: unhandled audio buffer format");
        break;
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "ANDROIDAUDIO_PlayDevice okay");
}

static Uint8 *
ANDROIDAUDIO_GetDeviceBuf(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;

    return audiodata->audioBufferPinned;
}

static void
ANDROIDAUDIO_CloseDevice(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;

    AudioTrack_Close( audiodata->audioTrack );

    if (this->iscapture) {
        SDL_assert(captureDevice == this);
        captureDevice = NULL;
    } else {
        SDL_assert(audioDevice == this);
        audioDevice = NULL;
    }
    SDL_free(audiodata);

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