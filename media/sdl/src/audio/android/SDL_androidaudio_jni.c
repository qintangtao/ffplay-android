#include "SDL_androidaudio_jni.h"
#include "SDL_stdinc.h"
#include "SDL_log.h"
#include "SDL_error.h"
#include "SDL_assert.h"

int Android_JNI_OpenAudioDevice(int iscapture, SDL_AudioSpec *spec, struct SDL_PrivateAudioData *data) {
    int audioformat;
    jobject jbufobj = NULL;
    jboolean isCopy;
    int result;
    SDL_Android_AudioTrack *track;

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
        track = AudioTrack_Open(SDL_TRUE, spec->freq, audioformat, spec->channels, spec->samples);
    } else {
        SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: opening device for output");
        track = AudioTrack_Open(SDL_FALSE, spec->freq, audioformat, spec->channels, spec->samples);
    }
    if (!track) {
        return SDL_SetError("Java-side initialization failed");
    }

    audioformat = AudioTrack_audioFormat(track);
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
        AudioTrack_Close(track);
        return SDL_SetError("Unexpected audio format from Java: %d\n", audioformat);
    }

    spec->freq = AudioTrack_sampleRate(track);
    spec->channels = AudioTrack_channelCount(track);
    spec->samples = AudioTrack_desiredFrames(track);


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
        AudioTrack_Close(track);
        return SDL_OutOfMemory();
    }

    if (iscapture) {
        data->captureBufferFormat = audioformat;
        data->captureBuffer = jbufobj;
    } else {
        data->audioBufferFormat = audioformat;
        data->audioBuffer = jbufobj;
    }

    if (!iscapture) {
        isCopy = JNI_FALSE;
        switch (audioformat) {
        case ENCODING_PCM_8BIT:
            data->audioBufferPinned = (*env)->GetByteArrayElements(env, (jbyteArray)data->audioBuffer, &isCopy);
            break;
        case ENCODING_PCM_16BIT:
            data->audioBufferPinned = (*env)->GetShortArrayElements(env, (jshortArray)data->audioBuffer, &isCopy);
            break;
        case ENCODING_PCM_FLOAT:
            data->audioBufferPinned = (*env)->GetFloatArrayElements(env, (jfloatArray)data->audioBuffer, &isCopy);
            break;
        }
    }

    data->audioTrack = track;

    return 0;
}


void * Android_JNI_GetAudioBuffer(struct SDL_PrivateAudioData *data)
{
    return data->audioBufferPinned;
}

void Android_JNI_WriteAudioBuffer(struct SDL_PrivateAudioData *data)
{
    JNIEnv *env = Android_JNI_GetEnv();

    switch (data->audioBufferFormat) {
    case ENCODING_PCM_8BIT:
        (*env)->ReleaseByteArrayElements(env, (jbyteArray)data->audioBuffer, (jbyte *)data->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteByteBuffer(data->audioTrack, (jbyteArray)data->audioBuffer);
        break;
    case ENCODING_PCM_16BIT:
        (*env)->ReleaseShortArrayElements(env, (jshortArray)data->audioBuffer, (jshort *)data->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteShortBuffer(data->audioTrack, (jshortArray)data->audioBuffer);
        break;
    case ENCODING_PCM_FLOAT:
        (*env)->ReleaseFloatArrayElements(env, (jfloatArray)data->audioBuffer, (jfloat *)data->audioBufferPinned, JNI_COMMIT);
        AudioTrack_WriteFloatBuffer(data->audioTrack, (jfloatArray)data->audioBuffer);
        break;
    default:
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: unhandled audio buffer format");
        break;
    }
}

void Android_JNI_CloseAudioDevice(struct SDL_PrivateAudioData *data)
{
    AudioTrack_Close( data->audioTrack );
}