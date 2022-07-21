#include "SDL_openslES.h"
#include "SDL_timer.h"
#include "SDL_assert.h"
#include "SDL_timer.h"
#include "SDL_log.h"
#include "../SDL_audio_c.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define SL_ANDROID_SPEAKER_STEREO (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT)
#define SL_ANDROID_SPEAKER_QUAD (SL_ANDROID_SPEAKER_STEREO | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT)
#define SL_ANDROID_SPEAKER_5DOT1 (SL_ANDROID_SPEAKER_QUAD | SL_SPEAKER_FRONT_CENTER  | SL_SPEAKER_LOW_FREQUENCY)
#define SL_ANDROID_SPEAKER_7DOT1 (SL_ANDROID_SPEAKER_5DOT1 | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT)

/* engine interfaces */
static SLObjectItf engineObject;
static SLEngineItf engineEngine;
/* output mix interfaces */
static SLObjectItf outputMixObject;

/* buffer queue player interfaces */
static SLObjectItf bqPlayerObject;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

/* recorder interfaces */
static SLObjectItf recorderObject;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

static void openslES_DestroyEngine(void)
{
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_DestroyEngine()");

    /* destroy output mix object, and invalidate all associated interfaces */
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }

    /* destroy engine object, and invalidate all associated interfaces */
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
}

static int
openslES_CreateEngine(void)
{
    SLresult result;

    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openSLES_CreateEngine()");

    /* create engine */
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "slCreateEngine failed: %d", result);
        goto error;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "slCreateEngine OK");

    /* realize the engine */
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RealizeEngine failed: %d", result);
        goto error;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "RealizeEngine OK");

    /* get the engine interface, which is needed in order to create other objects */
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "EngineGetInterface failed: %d", result);
        goto error;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "EngineGetInterface OK");

    /* create output mix */
    const SLInterfaceID ids[1] = { SL_IID_VOLUME };
    const SLboolean req[1] = { SL_BOOLEAN_FALSE };
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "CreateOutputMix failed: %d", result);
        goto error;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "CreateOutputMix OK");

    /* realize the output mix */
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RealizeOutputMix failed: %d", result);
        goto error;
    }
    return 1;

error:
    openslES_DestroyEngine();
    return 0;
}

static void
bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    struct SDL_PrivateAudioData *audiodata = (struct SDL_PrivateAudioData *) context;

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SLES: Playback Callback");
    SDL_SemPost(audiodata->playsem);
}

static void
openslES_DestroyPCMPlayer(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLresult result;

    /* set the player's state to 'stopped' */
    if (bqPlayerPlay != NULL) {
        result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        if (SL_RESULT_SUCCESS != result) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SetPlayState stopped failed: %d", result);
        }
    }

    /* destroy buffer queue audio player object, and invalidate all associated interfaces */
    if (bqPlayerObject != NULL) {

        (*bqPlayerObject)->Destroy(bqPlayerObject);

        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
    }

    if (audiodata->playsem) {
        SDL_DestroySemaphore(audiodata->playsem);
        audiodata->playsem = NULL;
    }

    if (audiodata->mixbuff) {
        SDL_free(audiodata->mixbuff);
    }
}

static int
openslES_CreatePCMPlayer(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLDataFormat_PCM format_pcm;
    SLresult result;
    int i;

#if 1
    /* Just go with signed 16-bit audio as it's the most compatible */
    this->spec.format = AUDIO_S16SYS;
#else
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    while (test_format != 0) {
        if (SDL_AUDIO_ISSIGNED(test_format) && SDL_AUDIO_ISINT(test_format)) {
            break;
        }
        test_format = SDL_NextAudioFormat();
    }

    if (test_format == 0) {
        /* Didn't find a compatible format : */
        SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO,  "No compatible audio format, using signed 16-bit audio" );
        test_format = AUDIO_S16SYS;
    }
    this->spec.format = test_format;
#endif

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&this->spec);

    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "Try to open %u hz %u bit chan %u %s samples %u",
          this->spec.freq, SDL_AUDIO_BITSIZE(this->spec.format),
          this->spec.channels, (this->spec.format & 0x1000) ? "BE" : "LE", this->spec.samples);

    /* configure audio source */
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NUM_BUFFERS };

    format_pcm.formatType    = SL_DATAFORMAT_PCM;
    format_pcm.numChannels   = this->spec.channels;
    format_pcm.samplesPerSec = this->spec.freq * 1000;  /* / kilo Hz to milli Hz */
    format_pcm.bitsPerSample = SDL_AUDIO_BITSIZE(this->spec.format);
    format_pcm.containerSize = SDL_AUDIO_BITSIZE(this->spec.format);

    if (SDL_AUDIO_ISBIGENDIAN(this->spec.format)) {
        format_pcm.endianness = SL_BYTEORDER_BIGENDIAN;
    } else {
        format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    }

    switch (this->spec.channels)
    {
    case 1:
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT;
        break;
    case 2:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_STEREO;
        break;
    case 3:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_STEREO | SL_SPEAKER_FRONT_CENTER;
        break;
    case 4:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_QUAD;
        break;
    case 5:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_QUAD | SL_SPEAKER_FRONT_CENTER;
        break;
    case 6:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_5DOT1;
        break;
    case 7:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_5DOT1 | SL_SPEAKER_BACK_CENTER;
        break;
    case 8:
        format_pcm.channelMask = SL_ANDROID_SPEAKER_7DOT1;
        break;
    default:
        /* Unknown number of channels, fall back to stereo */
        this->spec.channels = 2;
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        break;
    }


    /*
     typedef struct SLDataFormat_PCM_ {
        SLuint32        formatType;         //格式PCM
        SLuint32        numChannels;        //声道数
        SLuint32        samplesPerSec;      //采样率
        SLuint32        bitsPerSample;      //采样位数
        SLuint32        containerSize;      //包含位数
        SLuint32        channelMask;        //立体声
        SLuint32        endianness;         //结束标志位
    } SLDataFormat_PCM;
    */
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "SLDataFormat_PCM formatType:%u, numChannels:%u, samplesPerSec:%u, bitsPerSample:%u, containerSize:%u, channelMask:%u, endianness:%u",
          format_pcm.formatType, 
          format_pcm.numChannels,
          format_pcm.samplesPerSec,
          format_pcm.bitsPerSample,
          format_pcm.containerSize,
          format_pcm.channelMask,
          format_pcm.endianness
          );
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "SLDataFormat_PCM SL_DATAFORMAT_PCM:%u, SL_PCMSAMPLEFORMAT_FIXED_16:%u, SL_BYTEORDER_LITTLEENDIAN:%u, SL_BYTEORDER_BIGENDIAN:%u",
        SL_DATAFORMAT_PCM, SL_PCMSAMPLEFORMAT_FIXED_16, SL_BYTEORDER_LITTLEENDIAN, SL_BYTEORDER_BIGENDIAN
        );


    SLDataSource audioSrc = { &loc_bufq, &format_pcm };

    /* configure audio sink */
    SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
    SLDataSink audioSnk = { &loc_outmix, NULL };

    /* create audio player */
    const SLInterfaceID ids[2] = {
        SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        SL_IID_VOLUME
    };

    const SLboolean req[2] = {
        SL_BOOLEAN_TRUE,
        SL_BOOLEAN_FALSE,
    };

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "CreateAudioPlayer failed: %d", result);
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "CreateAudioPlayer OK");

    /* realize the player */
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RealizeAudioPlayer failed: %d", result);
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "RealizeAudioPlayer OK");

    /* get the play interface */
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SL_IID_PLAY interface get failed: %d", result);
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "SL_IID_PLAY OK");

    /* get the buffer queue interface */
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SL_IID_BUFFERQUEUE interface get failed: %d", result);
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "SL_IID_BUFFERQUEUE OK");

    /* register callback on the buffer queue */
    /* context is '(SDL_PrivateAudioData *)this->hidden' */
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this->hidden);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RegisterCallback failed: %d", result);
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "RegisterCallback OK");

#if 0
    /* get the volume interface */
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SL_IID_VOLUME interface get failed: %d", result);
        /* goto failed; */
    }
#endif

    /* Create the audio buffer semaphore */
    audiodata->playsem = SDL_CreateSemaphore(NUM_BUFFERS - 1);
    if (!audiodata->playsem) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "cannot create Semaphore!");
        goto failed;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "CreateSemaphore OK");

    /* Create the sound buffers */
    audiodata->mixbuff = (Uint8 *) SDL_malloc(NUM_BUFFERS * this->spec.size);
    if (audiodata->mixbuff == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "mixbuffer allocate - out of memory");
        goto failed;
    }

    for (i = 0; i < NUM_BUFFERS; i++) {
        audiodata->pmixbuff[i] = audiodata->mixbuff + i * this->spec.size;
    }

    /* set the player's state to playing */
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Play set state failed: %d", result);
        goto failed;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_CreatePCMPlayer OK");
    return 0;

failed:

    openslES_DestroyPCMPlayer(this);

    return SDL_SetError("Open device failed!");
}

static void
bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    struct SDL_PrivateAudioData *audiodata = (struct SDL_PrivateAudioData *) context;

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "SLES: Recording Callback");
    SDL_SemPost(audiodata->playsem);
}

static void
openslES_DestroyPCMRecorder(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLresult result;

    /* stop recording */
    if (recorderRecord != NULL) {
        result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
        if (SL_RESULT_SUCCESS != result) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SetRecordState stopped: %d", result);
        }
    }

    /* destroy audio recorder object, and invalidate all associated interfaces */
    if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recorderRecord = NULL;
        recorderBufferQueue = NULL;
    }

    if (audiodata->playsem) {
        SDL_DestroySemaphore(audiodata->playsem);
        audiodata->playsem = NULL;
    }

    if (audiodata->mixbuff) {
        SDL_free(audiodata->mixbuff);
    }
}

static int
openslES_CreatePCMRecorder(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLDataFormat_PCM format_pcm;
    SLresult result;
    int i;

    //if (!Android_JNI_RequestPermission("android.permission.RECORD_AUDIO")) {
    //    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "This app doesn't have RECORD_AUDIO permission");
    //    return SDL_SetError("This app doesn't have RECORD_AUDIO permission");
    //}

    /* Just go with signed 16-bit audio as it's the most compatible */
    this->spec.format = AUDIO_S16SYS;
    this->spec.channels = 1;
    /*this->spec.freq = SL_SAMPLINGRATE_16 / 1000;*/

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&this->spec);

    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "Try to open %u hz %u bit chan %u %s samples %u",
          this->spec.freq, SDL_AUDIO_BITSIZE(this->spec.format),
          this->spec.channels, (this->spec.format & 0x1000) ? "BE" : "LE", this->spec.samples);

    /* configure audio source */
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    /* configure audio sink */
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NUM_BUFFERS };

    format_pcm.formatType    = SL_DATAFORMAT_PCM;
    format_pcm.numChannels   = this->spec.channels;
    format_pcm.samplesPerSec = this->spec.freq * 1000;  /* / kilo Hz to milli Hz */
    format_pcm.bitsPerSample = SDL_AUDIO_BITSIZE(this->spec.format);
    format_pcm.containerSize = SDL_AUDIO_BITSIZE(this->spec.format);
    format_pcm.endianness    = SL_BYTEORDER_LITTLEENDIAN;
    format_pcm.channelMask   = SL_SPEAKER_FRONT_CENTER;

    SLDataSink audioSnk = { &loc_bufq, &format_pcm };

    /* create audio recorder */
    /* (requires the RECORD_AUDIO permission) */
    const SLInterfaceID ids[1] = {
        SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
    };
    const SLboolean req[1] = {
        SL_BOOLEAN_TRUE,
    };

    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "CreateAudioRecorder failed: %d", result);
        goto failed;
    }

    /* realize the recorder */
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RealizeAudioPlayer failed: %d", result);
        goto failed;
    }

    /* get the record interface */
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SL_IID_RECORD interface get failed: %d", result);
        goto failed;
    }

    /* get the buffer queue interface */
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SL_IID_BUFFERQUEUE interface get failed: %d", result);
        goto failed;
    }

    /* register callback on the buffer queue */
    /* context is '(SDL_PrivateAudioData *)this->hidden' */
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback, this->hidden);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "RegisterCallback failed: %d", result);
        goto failed;
    }

    /* Create the audio buffer semaphore */
    audiodata->playsem = SDL_CreateSemaphore(0);
    if (!audiodata->playsem) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "cannot create Semaphore!");
        goto failed;
    }

    /* Create the sound buffers */
    audiodata->mixbuff = (Uint8 *) SDL_malloc(NUM_BUFFERS * this->spec.size);
    if (audiodata->mixbuff == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "mixbuffer allocate - out of memory");
        goto failed;
    }

    for (i = 0; i < NUM_BUFFERS; i++) {
        audiodata->pmixbuff[i] = audiodata->mixbuff + i * this->spec.size;
    }

    /* in case already recording, stop recording and clear buffer queue */
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Record set state failed: %d", result);
        goto failed;
    }

    /* enqueue empty buffers to be filled by the recorder */
    for (i = 0; i < NUM_BUFFERS; i++) {
        result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audiodata->pmixbuff[i], this->spec.size);
        if (SL_RESULT_SUCCESS != result) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Record enqueue buffers failed: %d", result);
            goto failed;
        }
    }

    /* start recording */
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Record set state failed: %d", result);
        goto failed;
    }

    return 0;

failed:

    openslES_DestroyPCMRecorder(this);

    return SDL_SetError("Open device failed!");
}

static int
openslES_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    this->hidden = (struct SDL_PrivateAudioData *) SDL_calloc(1, (sizeof *this->hidden));
    if (this->hidden == NULL) {
        return -1;
    }

    if (iscapture) {
        SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_OpenDevice() %s for capture", devname);
        return openslES_CreatePCMRecorder(this);
        return -1;
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_OpenDevice() %s for playing", devname);
        return openslES_CreatePCMPlayer(this);
    }
}

static void
openslES_WaitDevice(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "openslES_WaitDevice()");

    /* Wait for an audio chunk to finish */
    SDL_SemWait(audiodata->playsem);
}

static void
openslES_PlayDevice(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLresult result;

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "======openslES_PlayDevice()======");

    /* Queue it up */
    result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, audiodata->pmixbuff[audiodata->next_buffer], this->spec.size);

    audiodata->next_buffer++;
    if (audiodata->next_buffer >= NUM_BUFFERS) {
        audiodata->next_buffer = 0;
    }

    /* If Enqueue fails, callback won't be called.
     * Post the semphore, not to run out of buffer */
    if (SL_RESULT_SUCCESS != result) {
        SDL_SemPost(audiodata->playsem);
    }
}

static Uint8 *
openslES_GetDeviceBuf(_THIS)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;

    SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "openslES_GetDeviceBuf()");
    return audiodata->pmixbuff[audiodata->next_buffer];
}

static int
openslES_CaptureFromDevice(_THIS, void *buffer, int buflen)
{
    struct SDL_PrivateAudioData *audiodata = this->hidden;
    SLresult result;

    /* Wait for new recorded data */
    SDL_SemWait(audiodata->playsem);

    /* Copy it to the output buffer */
    SDL_assert(buflen == this->spec.size);
    SDL_memcpy(buffer, audiodata->pmixbuff[audiodata->next_buffer], this->spec.size);

    /* Re-enqueue the buffer */
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, audiodata->pmixbuff[audiodata->next_buffer], this->spec.size);
    if (SL_RESULT_SUCCESS != result) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Record enqueue buffers failed: %d", result);
        return -1;
    }

    audiodata->next_buffer++;
    if (audiodata->next_buffer >= NUM_BUFFERS) {
        audiodata->next_buffer = 0;
    }

    return this->spec.size;
}


static void
openslES_CloseDevice(_THIS)
{
    /* struct SDL_PrivateAudioData *audiodata = this->hidden; */

    if (this->iscapture) {
        SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_CloseDevice() for capture");
        openslES_DestroyPCMRecorder(this);
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "openslES_CloseDevice() for playing");
        openslES_DestroyPCMPlayer(this);
    }

    SDL_free(this->hidden);
}


static int
openslES_Init(SDL_AudioDriverImpl * impl)
{
    if (!openslES_CreateEngine()) {
        return 0;
    }

    impl->OpenDevice    = openslES_OpenDevice;
    impl->WaitDevice    = openslES_WaitDevice;
    impl->PlayDevice    = openslES_PlayDevice;
    impl->GetDeviceBuf  = openslES_GetDeviceBuf;
    impl->CaptureFromDevice = openslES_CaptureFromDevice;
    impl->CloseDevice   = openslES_CloseDevice;
    impl->Deinitialize  = openslES_DestroyEngine;

    impl->HasCaptureSupport = 1;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultCaptureDevice = 1;

    return 1;
}

AudioBootStrap openslES_bootstrap = {
    "openslES", "opensl ES audio driver", openslES_Init, 0
};