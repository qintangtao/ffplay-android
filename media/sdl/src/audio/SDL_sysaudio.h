#ifndef SDL_sysaudio_h_
#define SDL_sysaudio_h_


#include "SDL_audio.h"
#include "SDL_stdinc.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "SDL_atomic.h"

#define DEFAULT_OUTPUT_DEVNAME "System audio output device"
#define DEFAULT_INPUT_DEVNAME "System audio capture device"

typedef struct SDL_AudioSpec SDL_AudioSpec;
typedef struct SDL_AudioDevice SDL_AudioDevice;

#define _THIS   SDL_AudioDevice *_this

#define SDL_AUDIOBUFFERQUEUE_PACKETLEN (8 * 1024)

typedef struct SDL_AudioDriverImpl
{
    void (*DetectDevices) (void);
    int (*OpenDevice) (_THIS, void *handle, const char *devname, int iscapture);
    void (*ThreadInit) (_THIS); /* Called by audio thread at start */
    void (*ThreadDeinit) (_THIS); /* Called by audio thread at end */
    void (*BeginLoopIteration)(_THIS);  /* Called by audio thread at top of loop */
    void (*WaitDevice) (_THIS);
    void (*PlayDevice) (_THIS);
    Uint8 *(*GetDeviceBuf) (_THIS);
    int (*CaptureFromDevice) (_THIS, void *buffer, int buflen);
    void (*FlushCapture) (_THIS);
    void (*PrepareToClose) (_THIS);  /**< Called between run and draining wait for playback devices */
    void (*CloseDevice) (_THIS);
    void (*LockDevice) (_THIS);
    void (*UnlockDevice) (_THIS);
    void (*FreeDeviceHandle) (void *handle);  /**< SDL is done with handle from SDL_AddAudioDevice() */
    void (*Deinitialize) (void);

    /* !!! FIXME: add pause(), so we can optimize instead of mixing silence. */

    /* Some flags to push duplicate code into the core and reduce #ifdefs. */
    /* !!! FIXME: these should be SDL_bool */
    int ProvidesOwnCallbackThread;
    int SkipMixerLock;
    int HasCaptureSupport;
    int OnlyHasDefaultOutputDevice;
    int OnlyHasDefaultCaptureDevice;
    int AllowsArbitraryDeviceNames;
} SDL_AudioDriverImpl;



typedef struct SDL_AudioDeviceItem
{
    void *handle;
    char *name;
    char *original_name;
    int dupenum;
    struct SDL_AudioDeviceItem *next;
} SDL_AudioDeviceItem;



typedef struct SDL_AudioDriver
{
    /* * * */
    /* The name of this audio driver */
    const char *name;

    /* * * */
    /* The description of this audio driver */
    const char *desc;

    SDL_AudioDriverImpl impl;

    /* A mutex for device detection */
    SDL_mutex *detectionLock;
    SDL_bool captureDevicesRemoved;
    SDL_bool outputDevicesRemoved;
    int outputDeviceCount;
    int inputDeviceCount;
    SDL_AudioDeviceItem *outputDevices;
    SDL_AudioDeviceItem *inputDevices;
} SDL_AudioDriver;

/* Define the SDL audio driver structure */
struct SDL_AudioDevice
{
    /* * * */
    /* Data common to all devices */
    SDL_AudioDeviceID id;

    /* The device's current audio specification */
    SDL_AudioSpec spec;

    /* The callback's expected audio specification (converted vs device's spec). */
    SDL_AudioSpec callbackspec;

    /* Stream that converts and resamples. NULL if not needed. */
    //SDL_AudioStream *stream;

    /* Current state flags */
    SDL_atomic_t shutdown; /* true if we are signaling the play thread to end. */
    SDL_atomic_t enabled;  /* true if device is functioning and connected. */
    SDL_atomic_t paused;
    SDL_bool iscapture;

    /* Scratch buffer used in the bridge between SDL and the user callback. */
    Uint8 *work_buffer;

    /* Size, in bytes, of work_buffer. */
    Uint32 work_buffer_len;

    /* A mutex for locking the mixing buffers */
    SDL_mutex *mixer_lock;

    /* A thread to feed the audio device */
    SDL_Thread *thread;
    SDL_threadID threadid;

    /* Queued buffers (if app not using callback). */
   // SDL_DataQueue *buffer_queue;

    /* * * */
    /* Data private to this driver */
    struct SDL_PrivateAudioData *hidden;

    void *handle;
};
#undef _THIS

typedef struct AudioBootStrap
{
    const char *name;
    const char *desc;
    int (*init) (SDL_AudioDriverImpl * impl);
    int demand_only;  /* 1==request explicitly, or it won't be available. */
} AudioBootStrap;

extern AudioBootStrap openslES_bootstrap;

extern AudioBootStrap ANDROIDAUDIO_bootstrap;

#endif /*SDL_sysaudio_h_*/