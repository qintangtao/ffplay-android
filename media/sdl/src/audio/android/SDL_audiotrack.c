#include "SDL_audiotrack.h"
#include "SDL_audiotrack_c.h"
#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_assert.h"
#include "j4a_base.h"
#include "j4a_allclasses.h"

int AudioTrack_sampleRate(SDL_Android_AudioTrack *track)
{
    return track->sampleRate;
}
int AudioTrack_audioFormat(SDL_Android_AudioTrack *track)
{
    return track->audioFormat;
}
int AudioTrack_channelCount(SDL_Android_AudioTrack *track)
{
    return track->channelCount;
}
int AudioTrack_desiredFrames(SDL_Android_AudioTrack *track)
{
    return track->desiredFrames;
}

SDL_Android_AudioTrack *AudioTrack_Open(SDL_bool isCapture, int sampleRate, int audioFormat, int desiredChannels, int desiredFrames) {
    int channelConfig;
    int sampleSize;
    int frameSize;
    int minBufferSize;
    int SDK_INT;
    jobject audioTrack;
    SDL_Android_AudioTrack *track;

    JNIEnv *env = Android_JNI_GetEnv();
    if (!env) {
        SDL_SetError("Android_JNI_GetEnv null");
        return NULL;
    }

    SDK_INT = J4A_GetSystemAndroidApiLevel(env);

    track = (SDL_Android_AudioTrack *) SDL_calloc(1, sizeof (SDL_Android_AudioTrack));

    track->isCapture = isCapture;

    //SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Opening " + (isCapture ? "capture" : "playback") + ", requested " + desiredFrames + " frames of " + desiredChannels + " channel " + getAudioFormatString(audioFormat) + " audio at " + sampleRate + " Hz");

    /* On older devices let's use known good settings */
    if (SDK_INT < 21) {
        if (desiredChannels > 2) {
            desiredChannels = 2;
        }
        if (sampleRate < 8000) {
            sampleRate = 8000;
        } else if (sampleRate > 48000) {
            sampleRate = 48000;
        }
    }

    if (audioFormat == ENCODING_PCM_FLOAT) {
        int minSDKVersion = (isCapture ? 23 : 21);
        if (SDK_INT < minSDKVersion) {
            audioFormat = ENCODING_PCM_16BIT;
        }
    }

    switch (audioFormat)
    {
    case ENCODING_PCM_8BIT:
        sampleSize = 1;
        break;
    case ENCODING_PCM_16BIT:
        sampleSize = 2;
        break;
    case ENCODING_PCM_FLOAT:
        sampleSize = 4;
        break;
    default:
        SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Requested format %d, getting ENCODING_PCM_16BIT", audioFormat);
        audioFormat = ENCODING_PCM_16BIT;
        sampleSize = 2;
        break;
    }

    if (isCapture) {
        switch (desiredChannels) {
        case 1:
            channelConfig = CHANNEL_IN_MONO;
            break;
        case 2:
            channelConfig = CHANNEL_IN_STEREO;
            break;
        default:
            SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Requested %d channels, getting stereo", desiredChannels);
            desiredChannels = 2;
            channelConfig = CHANNEL_IN_STEREO;
            break;
        }
    } else {
        switch (desiredChannels) {
        case 1:
            channelConfig = CHANNEL_OUT_MONO;
            break;
        case 2:
            channelConfig = CHANNEL_OUT_STEREO;
            break;
        case 3:
            channelConfig = CHANNEL_OUT_STEREO | CHANNEL_OUT_FRONT_CENTER;
            break;
        case 4:
            channelConfig = CHANNEL_OUT_QUAD;
            break;
        case 5:
            channelConfig = CHANNEL_OUT_QUAD | CHANNEL_OUT_FRONT_CENTER;
            break;
        case 6:
            channelConfig = CHANNEL_OUT_5POINT1;
            break;
        case 7:
            channelConfig = CHANNEL_OUT_5POINT1 | CHANNEL_OUT_BACK_CENTER;
            break;
        case 8:
            if (SDK_INT >= 23) {
                channelConfig = CHANNEL_OUT_7POINT1_SURROUND;
            } else {
                SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Requested %d channels, getting 5.1 surround", desiredChannels);
                desiredChannels = 6;
                channelConfig = CHANNEL_OUT_5POINT1;
            }
            break;
        default:
            SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Requested %d channels, getting stereo", desiredChannels);
            desiredChannels = 2;
            channelConfig = CHANNEL_OUT_STEREO;
            break;
        }
    }

    frameSize = (sampleSize * desiredChannels);
    if (isCapture) {
        minBufferSize = 0;
        //minBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat);
    } else {
        minBufferSize = J4AC_AudioTrack__getMinBufferSize__catchAll(env, sampleRate, channelConfig, audioFormat);
    }
    desiredFrames = SDL_max(desiredFrames, (minBufferSize + frameSize - 1) / frameSize);

    if (isCapture) {

    } else {
        audioTrack = J4AC_AudioTrack__AudioTrack__asGlobalRef__catchAll(
                        env, STREAM_MUSIC, sampleRate, channelConfig, audioFormat, desiredFrames * frameSize, MODE_STREAM);

        if (J4AC_AudioTrack__getState__catchAll(env, audioTrack) !=  STATE_INITIALIZED) {
            J4AC_AudioTrack__release__catchAll(env, audioTrack);
            return 0;
        }

        J4AC_AudioTrack__play__catchAll(env, audioTrack);

        track->audioTrack = audioTrack;
        track->sampleRate = J4AC_AudioTrack__getSampleRate__catchAll(env, audioTrack);
        track->audioFormat = J4AC_AudioTrack__getAudioFormat__catchAll(env, audioTrack);
        track->channelCount = J4AC_AudioTrack__getChannelCount__catchAll(env, audioTrack);
    }

    track->desiredFrames = desiredFrames;

    return track;
}

void AudioTrack_Close( SDL_Android_AudioTrack *track)
{
    JNIEnv *env = Android_JNI_GetEnv();

    SDL_assert(track != NULL);

    if (track->isCapture)
    {

    }
    else
    {
       SDL_assert(track->audioTrack != NULL);
       J4AC_AudioTrack__stop__catchAll(env, track->audioTrack);
       J4AC_AudioTrack__release__catchAll(env, track->audioTrack);
    }

    SDL_free(track);
}

void AudioTrack_WriteByteBuffer(SDL_Android_AudioTrack *track, jbyteArray buffer)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (!env) {
        SDL_SetError("Android_JNI_GetEnv null");
        return;
    }

    jsize length = (*env)->GetArrayLength(env, buffer);
    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write byte buffer [%u:%u]", length, 0);

    int retval = J4AC_AudioTrack__write(env, track->audioTrack, buffer, 0, length);
    if (J4A_ExceptionCheck__catchAll(env))
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write byte buffer error");

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write byte buffer [%u:%u]", length, retval);
}

void AudioTrack_WriteShortBuffer(SDL_Android_AudioTrack *track, jshortArray buffer)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (!env) {
        SDL_SetError("Android_JNI_GetEnv null");
        return;
    }

    jsize length = (*env)->GetArrayLength(env, buffer);
    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write short buffer [%u:%u]", length, 0);

    int retval = J4AC_AudioTrack__write_short(env, track->audioTrack, buffer, 0, length);
    if (J4A_ExceptionCheck__catchAll(env))
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write byte short error");

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write short buffer [%u:%u]", length, retval);
}

void AudioTrack_WriteFloatBuffer(SDL_Android_AudioTrack *track, jfloatArray buffer)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (!env) {
        SDL_SetError("Android_JNI_GetEnv null");
        return;
    }

    jsize length = (*env)->GetArrayLength(env, buffer);
    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write float buffer [%u:%u]", length, 0);

    int retval = J4AC_AudioTrack__write_float(env, track->audioTrack, buffer, 0, length, 0);
    if (J4A_ExceptionCheck__catchAll(env))
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write float buffer error");

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SDL audio: audio track write float buffer [%u:%u]", length, retval);
}
