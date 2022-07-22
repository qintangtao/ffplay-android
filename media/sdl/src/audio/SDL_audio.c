#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_sysaudio.h"
#include "SDL_mutex.h"
#include "SDL_assert.h"
#include "SDL_timer.h"
#include "SDL_log.h"
#include "../thread/SDL_thread_c.h"

#define _THIS SDL_AudioDevice *_this

static SDL_AudioDriver current_audio;
static SDL_AudioDevice *open_devices[16];

static const AudioBootStrap *const bootstrap[] = {
    &openslES_bootstrap,
#if __ANDROID__
    &ANDROIDAUDIO_bootstrap,
#endif
    NULL
};


static SDL_AudioDevice *
get_audio_device(SDL_AudioDeviceID id)
{
    id--;
    if ((id >= SDL_arraysize(open_devices)) || (open_devices[id] == NULL)) {
        SDL_SetError("Invalid audio device ID");
        //SDL_InvalidParamError("id");
        return NULL;
    }

    return open_devices[id];
}

static int SDLCALL
SDL_RunAudio(void *devicep)
{
    SDL_AudioDevice *device = (SDL_AudioDevice *) devicep;
    void *udata = device->callbackspec.userdata;
    SDL_AudioCallback callback = device->callbackspec.callback;
    int data_len = 0;
    Uint8 *data;

    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "SDL_RunAudio enter");

#ifdef __ANDROID__
    {
        /* Set thread priority to THREAD_PRIORITY_AUDIO */
        extern void Android_JNI_AudioSetThreadPriority(int, int);
        Android_JNI_AudioSetThreadPriority(device->iscapture, device->id);
    }
#else
    /* The audio mixing is always a high priority thread */
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
#endif

    device->threadid = SDL_ThreadID();
    current_audio.impl.ThreadInit(device);

    /* Loop, filling the audio buffers */
    while (!SDL_AtomicGet(&device->shutdown)) {
        current_audio.impl.BeginLoopIteration(device);
        data_len = device->callbackspec.size;

        if (/*!device->stream && */SDL_AtomicGet(&device->enabled)) {
            SDL_assert(data_len == device->spec.size);
            data = current_audio.impl.GetDeviceBuf(device);
        } else {
            data = NULL;
        }

        if (data == NULL) {
            data = device->work_buffer;
        }

        SDL_LockMutex(device->mixer_lock);
        if (SDL_AtomicGet(&device->paused)) {
            SDL_memset(data, device->callbackspec.silence, data_len);
        } else {
            callback(udata, data, data_len);
        }
        SDL_UnlockMutex(device->mixer_lock);

        if (data == device->work_buffer) {
            /* nothing to do; pause like we queued a buffer to play. */
            const Uint32 delay = ((device->spec.samples * 1000) / device->spec.freq);
            SDL_Delay(delay);
        } else {  /* writing directly to the device. */
            /* queue this buffer and wait for it to finish playing. */
            current_audio.impl.PlayDevice(device);
            current_audio.impl.WaitDevice(device);
        }
    }

    current_audio.impl.PrepareToClose(device);

    /* Wait for the audio to drain. */
    SDL_Delay(((device->spec.samples * 1000) / device->spec.freq) * 2);

    current_audio.impl.ThreadDeinit(device);

    return 0;
}

static int SDLCALL
SDL_CaptureAudio(void *devicep)
{
    SDL_AudioDevice *device = (SDL_AudioDevice *) devicep;
    const int silence = (int) device->spec.silence;
    const Uint32 delay = ((device->spec.samples * 1000) / device->spec.freq);
    const int data_len = device->spec.size;
    Uint8 *data;
    void *udata = device->callbackspec.userdata;
    SDL_AudioCallback callback = device->callbackspec.callback;

    SDL_assert(device->iscapture);

#ifdef __ANDROID__
    {
        /* Set thread priority to THREAD_PRIORITY_AUDIO */
        extern void Android_JNI_AudioSetThreadPriority(int, int);
        Android_JNI_AudioSetThreadPriority(device->iscapture, device->id);
    }
#else
    /* The audio mixing is always a high priority thread */
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
#endif

    /* Perform any thread setup */
    device->threadid = SDL_ThreadID();
    current_audio.impl.ThreadInit(device);

    /* Loop, filling the audio buffers */
    while (!SDL_AtomicGet(&device->shutdown)) {
        int still_need;
        Uint8 *ptr;

        current_audio.impl.BeginLoopIteration(device);

        if (SDL_AtomicGet(&device->paused)) {
            SDL_Delay(delay);  /* just so we don't cook the CPU. */
            //if (device->stream) {
            //    SDL_AudioStreamClear(device->stream);
            //}
            current_audio.impl.FlushCapture(device);  /* dump anything pending. */
            continue;
        }

        /* Fill the current buffer with sound */
        still_need = data_len;

        /* Use the work_buffer to hold data read from the device. */
        data = device->work_buffer;
        SDL_assert(data != NULL);

        ptr = data;

        /* We still read from the device when "paused" to keep the state sane,
           and block when there isn't data so this thread isn't eating CPU.
           But we don't process it further or call the app's callback. */

        if (!SDL_AtomicGet(&device->enabled)) {
            SDL_Delay(delay);  /* try to keep callback firing at normal pace. */
        } else {
            while (still_need > 0) {
                const int rc = current_audio.impl.CaptureFromDevice(device, ptr, still_need);
                SDL_assert(rc <= still_need);  /* device should not overflow buffer. :) */
                if (rc > 0) {
                    still_need -= rc;
                    ptr += rc;
                } else {  /* uhoh, device failed for some reason! */
                    //SDL_OpenedAudioDeviceDisconnected(device);
                    break;
                }
            }
        }

        if (still_need > 0) {
            /* Keep any data we already read, silence the rest. */
            SDL_memset(ptr, silence, still_need);
        }

        SDL_LockMutex(device->mixer_lock);
        if (!SDL_AtomicGet(&device->paused)) {
            callback(udata, data, device->callbackspec.size);
        }
        SDL_UnlockMutex(device->mixer_lock);
    }

    current_audio.impl.FlushCapture(device);

    current_audio.impl.ThreadDeinit(device);
    return 0;
}


static void
close_audio_device(SDL_AudioDevice * device)
{
    if (!device) {
        return;
    }

    current_audio.impl.LockDevice(device);
    SDL_AtomicSet(&device->paused, 1);
    SDL_AtomicSet(&device->shutdown, 1);
    SDL_AtomicSet(&device->enabled, 0);
    current_audio.impl.UnlockDevice(device);

    if (device->thread != NULL) {
        SDL_WaitThread(device->thread, NULL);
    }
    if (device->mixer_lock != NULL) {
        SDL_DestroyMutex(device->mixer_lock);
    }

    SDL_free(device->work_buffer);
    //SDL_FreeAudioStream(device->stream);

    if (device->id > 0) {
        SDL_AudioDevice *opendev = open_devices[device->id - 1];
        SDL_assert((opendev == device) || (opendev == NULL));
        if (opendev == device) {
            open_devices[device->id - 1] = NULL;
        }
    }

    if (device->hidden != NULL) {
        current_audio.impl.CloseDevice(device);
    }

    //SDL_FreeDataQueue(device->buffer_queue);

    SDL_free(device);
}

static SDL_AudioFormat
SDL_ParseAudioFormat(const char *string)
{
#define CHECK_FMT_STRING(x) if (SDL_strcmp(string, #x) == 0) return AUDIO_##x
    CHECK_FMT_STRING(U8);
    CHECK_FMT_STRING(S8);
    CHECK_FMT_STRING(U16LSB);
    CHECK_FMT_STRING(S16LSB);
    CHECK_FMT_STRING(U16MSB);
    CHECK_FMT_STRING(S16MSB);
    CHECK_FMT_STRING(U16SYS);
    CHECK_FMT_STRING(S16SYS);
    CHECK_FMT_STRING(U16);
    CHECK_FMT_STRING(S16);
    CHECK_FMT_STRING(S32LSB);
    CHECK_FMT_STRING(S32MSB);
    CHECK_FMT_STRING(S32SYS);
    CHECK_FMT_STRING(S32);
    CHECK_FMT_STRING(F32LSB);
    CHECK_FMT_STRING(F32MSB);
    CHECK_FMT_STRING(F32SYS);
    CHECK_FMT_STRING(F32);
#undef CHECK_FMT_STRING
    return 0;
}

static int
prepare_audiospec(const SDL_AudioSpec * orig, SDL_AudioSpec * prepared)
{
    SDL_memcpy(prepared, orig, sizeof(SDL_AudioSpec));

    if (orig->freq == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FREQUENCY");
        if ((!env) || ((prepared->freq = SDL_atoi(env)) == 0)) {
            prepared->freq = 22050;     /* a reasonable default */
        }
    }

    if (orig->format == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FORMAT");
        if ((!env) || ((prepared->format = SDL_ParseAudioFormat(env)) == 0)) {
            prepared->format = AUDIO_S16;       /* a reasonable default */
        }
    }

    switch (orig->channels) {
    case 0:{
            const char *env = SDL_getenv("SDL_AUDIO_CHANNELS");
            if ((!env) || ((prepared->channels = (Uint8) SDL_atoi(env)) == 0)) {
                prepared->channels = 2; /* a reasonable default */
            }
            break;
        }
    case 1:                    /* Mono */
    case 2:                    /* Stereo */
    case 4:                    /* Quadrophonic */
    case 6:                    /* 5.1 surround */
    case 8:                    /* 7.1 surround */
        break;
    default:
        SDL_SetError("Unsupported number of audio channels.");
        return 0;
    }

    if (orig->samples == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_SAMPLES");
        if ((!env) || ((prepared->samples = (Uint16) SDL_atoi(env)) == 0)) {
            /* Pick a default of ~46 ms at desired frequency */
            const int samples = (prepared->freq / 1000) * 46;
            int power2 = 1;
            while (power2 < samples) {
                power2 *= 2;
            }
            prepared->samples = power2;
        }
    }

    /* Calculate the silence and size of the audio specification */
    SDL_CalculateAudioSpec(prepared);

    return 1;
}

static SDL_AudioDeviceID
open_audio_device(const char *devname, int iscapture,
                  const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                  int allowed_changes, int min_id)
{
    const SDL_bool is_internal_thread = (desired->callback == NULL);
    SDL_AudioDeviceID id = 0;
    SDL_AudioSpec _obtained;
    SDL_AudioDevice *device;
    SDL_bool build_stream;
    void *handle = NULL;
    int i = 0;

    if (!current_audio.impl.OpenDevice) {
        SDL_SetError("Audio subsystem is not initialized");
        return 0;
    }

    if (iscapture && !current_audio.impl.HasCaptureSupport) {
        SDL_SetError("No capture support");
        return 0;
    }

    for (id = min_id - 1; id < SDL_arraysize(open_devices); id++) {
        if (open_devices[id] == NULL) {
            break;
        }
    }

    if (id == SDL_arraysize(open_devices)) {
        SDL_SetError("Too many open audio devices");
        return 0;
    }

    if (!obtained) {
        obtained = &_obtained;
    }
    if (!prepare_audiospec(desired, obtained)) {
        return 0;
    }

    /* If app doesn't care about a specific device, let the user override. */
    if (devname == NULL) {
        devname = SDL_getenv("SDL_AUDIO_DEVICE_NAME");
    }

    if ((iscapture) && (current_audio.impl.OnlyHasDefaultCaptureDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_INPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    } else if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_OUTPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (!open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    } else if (devname != NULL) {
        SDL_AudioDeviceItem *item;
        SDL_LockMutex(current_audio.detectionLock);
        for (item = iscapture ? current_audio.inputDevices : current_audio.outputDevices; item; item = item->next) {
            if ((item->handle != NULL) && (SDL_strcmp(item->name, devname) == 0)) {
                handle = item->handle;
                break;
            }
        }
        SDL_UnlockMutex(current_audio.detectionLock);
    }

    if (!current_audio.impl.AllowsArbitraryDeviceNames) {
        if ((handle == NULL) && (devname != NULL)) {
            SDL_SetError("No such device.");
            return 0;
        }
    }

    device = (SDL_AudioDevice *) SDL_calloc(1, sizeof (SDL_AudioDevice));
    if (device == NULL) {
        return 0;
    }
    device->id = id + 1;
    device->spec = *obtained;
    device->iscapture = iscapture ? SDL_TRUE : SDL_FALSE;
    device->handle = handle;

    SDL_AtomicSet(&device->shutdown, 0);  /* just in case. */
    SDL_AtomicSet(&device->paused, 1);
    SDL_AtomicSet(&device->enabled, 1);

    if (!current_audio.impl.SkipMixerLock) {
        device->mixer_lock = SDL_CreateMutex();
        if (device->mixer_lock == NULL) {
            close_audio_device(device);
            SDL_SetError("Couldn't create mixer lock");
            return 0;
        }
    }

    if (current_audio.impl.OpenDevice(device, handle, devname, iscapture) < 0) {
        close_audio_device(device);
        return 0;
    }

    build_stream = SDL_FALSE;
    if (obtained->freq != device->spec.freq) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) {
            obtained->freq = device->spec.freq;
        } else {
            build_stream = SDL_TRUE;
        }
    }
    if (obtained->format != device->spec.format) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FORMAT_CHANGE) {
            obtained->format = device->spec.format;
        } else {
            build_stream = SDL_TRUE;
        }
    }
    if (obtained->channels != device->spec.channels) {
        if (allowed_changes & SDL_AUDIO_ALLOW_CHANNELS_CHANGE) {
            obtained->channels = device->spec.channels;
        } else {
            build_stream = SDL_TRUE;
        }
    }
    if (device->spec.samples != obtained->samples) {
        if (allowed_changes & SDL_AUDIO_ALLOW_SAMPLES_CHANGE) {
            obtained->samples = device->spec.samples;
        } else {
            build_stream = SDL_TRUE;
        }
    }

    SDL_CalculateAudioSpec(obtained);

    device->callbackspec = *obtained;

    /* Allocate a scratch audio buffer */
    device->work_buffer_len = build_stream ? device->callbackspec.size : 0;
    if (device->spec.size > device->work_buffer_len) {
        device->work_buffer_len = device->spec.size;
    }
    SDL_assert(device->work_buffer_len > 0);

    device->work_buffer = (Uint8 *) SDL_malloc(device->work_buffer_len);
    if (device->work_buffer == NULL) {
        close_audio_device(device);
        SDL_OutOfMemory();
        return 0;
    }

    open_devices[id] = device;

    if (!current_audio.impl.ProvidesOwnCallbackThread) {
        const size_t stacksize = is_internal_thread ? 64 * 1024 : 0;
        char threadname[64];

        SDL_snprintf(threadname, sizeof (threadname), "SDLAudio%c%d", (iscapture) ? 'C' : 'P', (int) device->id);
        device->thread = SDL_CreateThreadInternal(iscapture ? SDL_CaptureAudio : SDL_RunAudio, threadname, stacksize, device);

        if (device->thread == NULL) {
            close_audio_device(device);
            SDL_SetError("Couldn't create audio thread");
            return 0;
        }
    }

    return device->id;
}

static void
SDL_AudioDetectDevices_Default(void)
{
    /* you have to write your own implementation if these assertions fail. */
    SDL_assert(current_audio.impl.OnlyHasDefaultOutputDevice);
    SDL_assert(current_audio.impl.OnlyHasDefaultCaptureDevice || !current_audio.impl.HasCaptureSupport);

    //SDL_AddAudioDevice(SDL_FALSE, DEFAULT_OUTPUT_DEVNAME, (void *) ((size_t) 0x1));
    //if (current_audio.impl.HasCaptureSupport) {
    //    SDL_AddAudioDevice(SDL_TRUE, DEFAULT_INPUT_DEVNAME, (void *) ((size_t) 0x2));
    //}
}

static void
SDL_AudioThreadInit_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioThreadDeinit_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioBeginLoopIteration_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioWaitDevice_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioPlayDevice_Default(_THIS)
{                               /* no-op. */
}

static Uint8 *
SDL_AudioGetDeviceBuf_Default(_THIS)
{
    return NULL;
}

static int
SDL_AudioCaptureFromDevice_Default(_THIS, void *buffer, int buflen)
{
    return -1;  /* just fail immediately. */
}

static void
SDL_AudioFlushCapture_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioPrepareToClose_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioCloseDevice_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioDeinitialize_Default(void)
{                               /* no-op. */
}

static void
SDL_AudioFreeDeviceHandle_Default(void *handle)
{                               /* no-op. */
}


static int
SDL_AudioOpenDevice_Default(_THIS, void *handle, const char *devname, int iscapture)
{
    return SDL_Unsupported();
}

static inline SDL_bool
is_in_audio_device_thread(SDL_AudioDevice * device)
{
    if (device->thread && (SDL_ThreadID() == device->threadid)) {
        return SDL_TRUE;
    }

    return SDL_FALSE;
}

static void
SDL_AudioLockDevice_Default(SDL_AudioDevice * device)
{
    if (!is_in_audio_device_thread(device)) {
        SDL_LockMutex(device->mixer_lock);
    }
}

static void
SDL_AudioUnlockDevice_Default(SDL_AudioDevice * device)
{
    if (!is_in_audio_device_thread(device)) {
        SDL_UnlockMutex(device->mixer_lock);
    }
}

static void
SDL_AudioLockOrUnlockDeviceWithNoMixerLock(SDL_AudioDevice * device)
{
}

static void
finish_audio_entry_points_init(void)
{
    if (current_audio.impl.SkipMixerLock) {
        if (current_audio.impl.LockDevice == NULL) {
            current_audio.impl.LockDevice = SDL_AudioLockOrUnlockDeviceWithNoMixerLock;
        }
        if (current_audio.impl.UnlockDevice == NULL) {
            current_audio.impl.UnlockDevice = SDL_AudioLockOrUnlockDeviceWithNoMixerLock;
        }
    }

#define FILL_STUB(x) \
        if (current_audio.impl.x == NULL) { \
            current_audio.impl.x = SDL_Audio##x##_Default; \
        }
    FILL_STUB(DetectDevices);
    FILL_STUB(OpenDevice);
    FILL_STUB(ThreadInit);
    FILL_STUB(ThreadDeinit);
    FILL_STUB(BeginLoopIteration);
    FILL_STUB(WaitDevice);
    FILL_STUB(PlayDevice);
    FILL_STUB(GetDeviceBuf);
    FILL_STUB(CaptureFromDevice);
    FILL_STUB(FlushCapture);
    FILL_STUB(PrepareToClose);
    FILL_STUB(CloseDevice);
    FILL_STUB(LockDevice);
    FILL_STUB(UnlockDevice);
    FILL_STUB(FreeDeviceHandle);
    FILL_STUB(Deinitialize);
#undef FILL_STUB
}


int
SDL_AudioInit(const char *driver_name)
{
    int i = 0;
    int initialized = 0;
    int tried_to_init = 0;

    SDL_zero(current_audio);
    SDL_zeroa(open_devices);

    /* Select the proper audio driver */
    if (driver_name == NULL) {
        driver_name = SDL_getenv("SDL_AUDIODRIVER");
    }

    for (i = 0; (!initialized) && (bootstrap[i]); ++i) {
        /* make sure we should even try this driver before doing so... */
        const AudioBootStrap *backend = bootstrap[i];
        if ((driver_name && (SDL_strncasecmp(backend->name, driver_name, SDL_strlen(driver_name)) != 0)) ||
            (!driver_name && backend->demand_only)) {
            continue;
        }

        tried_to_init = 1;
        SDL_zero(current_audio);
        current_audio.name = backend->name;
        current_audio.desc = backend->desc;
        initialized = backend->init(&current_audio.impl);
    }

    if (!initialized) {
        /* specific drivers will set the error message if they fail... */
        if (!tried_to_init) {
            if (driver_name) {
                SDL_SetError("Audio target '%s' not available", driver_name);
            } else {
                SDL_SetError("No available audio device");
            }
        }

        SDL_zero(current_audio);
        return -1;            /* No driver was available, so fail. */
    }

    current_audio.detectionLock = SDL_CreateMutex();

    finish_audio_entry_points_init();

    /* Make sure we have a list of devices available at startup. */
    current_audio.impl.DetectDevices();

    return 0;
}

void
SDL_AudioQuit(void)
{
    SDL_AudioDeviceID i;

    if (!current_audio.name) {  /* not initialized?! */
        return;
    }

    for (i = 0; i < SDL_arraysize(open_devices); i++) {
        close_audio_device(open_devices[i]);
    }

    //free_device_list(&current_audio.outputDevices, &current_audio.outputDeviceCount);
    //free_device_list(&current_audio.inputDevices, &current_audio.inputDeviceCount);

    /* Free the driver data */
    current_audio.impl.Deinitialize();

    SDL_DestroyMutex(current_audio.detectionLock);

    SDL_zero(current_audio);
    SDL_zeroa(open_devices);
}


SDL_AudioDeviceID
SDL_OpenAudioDevice(const char *device, int iscapture,
                    const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                    int allowed_changes)
{
    return open_audio_device(device, iscapture, desired, obtained,
                             allowed_changes, 2);
}

void
SDL_CloseAudioDevice(SDL_AudioDeviceID devid)
{
    close_audio_device(get_audio_device(devid));
}

void
SDL_PauseAudioDevice(SDL_AudioDeviceID devid, int pause_on)
{
    SDL_AudioDevice *device = get_audio_device(devid);
    if (device) {
        current_audio.impl.LockDevice(device);
        SDL_AtomicSet(&device->paused, pause_on ? 1 : 0);
        current_audio.impl.UnlockDevice(device);
    }
}



#define NUM_FORMATS 10
static int format_idx;
static int format_idx_sub;
static SDL_AudioFormat format_list[NUM_FORMATS][NUM_FORMATS] = {
    {AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
};

SDL_AudioFormat
SDL_FirstAudioFormat(SDL_AudioFormat format)
{
    for (format_idx = 0; format_idx < NUM_FORMATS; ++format_idx) {
        if (format_list[format_idx][0] == format) {
            break;
        }
    }
    format_idx_sub = 0;
    return SDL_NextAudioFormat();
}

SDL_AudioFormat
SDL_NextAudioFormat(void)
{
    if ((format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS)) {
        return 0;
    }
    return format_list[format_idx][format_idx_sub++];
}

Uint8
SDL_SilenceValueForFormat(const SDL_AudioFormat format)
{
    switch (format) {
        case AUDIO_U16LSB:
        case AUDIO_U16MSB:
        case AUDIO_U8:
            return 0x80;
        default: break;
    }

    return 0x00;
}

void
SDL_CalculateAudioSpec(SDL_AudioSpec * spec)
{
    spec->silence = SDL_SilenceValueForFormat(spec->format);
    spec->size = SDL_AUDIO_BITSIZE(spec->format) / 8;
    spec->size *= spec->channels;
    spec->size *= spec->samples;
}