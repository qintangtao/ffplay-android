#ifndef SDL_audio_track_h_
#define SDL_audio_track_h_

#include "SDL_stdinc.h"
#include "../../core/android/SDL_android.h"

typedef enum {
    STATE_UNINITIALIZED = 0,
    STATE_INITIALIZED   = 1,
    STATE_NO_STATIC_DATA = 2
} AudioTrackState;

typedef enum  {
        ENCODING_INVALID = 0,
        ENCODING_DEFAULT = 1,
        ENCODING_PCM_16BIT = 2, // signed, guaranteed to be supported by devices.
        ENCODING_PCM_8BIT = 3, // unsigned, not guaranteed to be supported by devices.
        ENCODING_PCM_FLOAT = 4, // single-precision floating-point per sample
} AudioFormat;

typedef struct SDL_Android_AudioTrack_Spec {

    enum  StreamType{
            STREAM_VOICE_CALL = 0,
            STREAM_SYSTEM = 1,
            STREAM_RING = 2,
            STREAM_MUSIC = 3,
            STREAM_ALARM = 4,
            STREAM_NOTIFICATION = 5,

    } stream_type;


    int sample_rate_in_hz;

    enum ChannelConfig {
        CHANNEL_INVALID = 0x0,

        CHANNEL_IN_DEFAULT = 1,
        CHANNEL_IN_LEFT = 0x4,
        CHANNEL_IN_RIGHT = 0x8,
        CHANNEL_IN_FRONT = 0x10,
        CHANNEL_IN_BACK = 0x20,
        CHANNEL_IN_LEFT_PROCESSED = 0x40,
        CHANNEL_IN_RIGHT_PROCESSED = 0x80,
        CHANNEL_IN_FRONT_PROCESSED = 0x100,
        CHANNEL_IN_BACK_PROCESSED = 0x200,
        CHANNEL_IN_PRESSURE = 0x400,
        CHANNEL_IN_X_AXIS = 0x800,
        CHANNEL_IN_Y_AXIS = 0x1000,
        CHANNEL_IN_Z_AXIS = 0x2000,
        CHANNEL_IN_VOICE_UPLINK = 0x4000,
        CHANNEL_IN_VOICE_DNLINK = 0x8000,
        CHANNEL_IN_MONO = CHANNEL_IN_FRONT,
        CHANNEL_IN_STEREO = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT),
        CHANNEL_IN_FRONT_BACK = CHANNEL_IN_FRONT | CHANNEL_IN_BACK,

        CHANNEL_OUT_DEFAULT = 1,
        CHANNEL_OUT_FRONT_LEFT = 0x4,
        CHANNEL_OUT_FRONT_RIGHT = 0x8,
        CHANNEL_OUT_FRONT_CENTER = 0x10,
        CHANNEL_OUT_LOW_FREQUENCY = 0x20,
        CHANNEL_OUT_BACK_LEFT = 0x40,
        CHANNEL_OUT_BACK_RIGHT = 0x80,
        CHANNEL_OUT_FRONT_LEFT_OF_CENTER = 0x100,
        CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x200,
        CHANNEL_OUT_BACK_CENTER = 0x400,
        CHANNEL_OUT_SIDE_LEFT =         0x800,
        CHANNEL_OUT_SIDE_RIGHT =       0x1000,
        CHANNEL_OUT_TOP_CENTER =       0x2000,
        CHANNEL_OUT_TOP_FRONT_LEFT =   0x4000,
        CHANNEL_OUT_TOP_FRONT_CENTER = 0x8000,
        CHANNEL_OUT_TOP_FRONT_RIGHT = 0x10000,
        CHANNEL_OUT_TOP_BACK_LEFT =   0x20000,
        CHANNEL_OUT_TOP_BACK_CENTER = 0x40000,
        CHANNEL_OUT_TOP_BACK_RIGHT =  0x80000,
        CHANNEL_OUT_TOP_SIDE_LEFT = 0x100000,
        CHANNEL_OUT_TOP_SIDE_RIGHT = 0x200000,
        CHANNEL_OUT_BOTTOM_FRONT_LEFT = 0x400000,
        CHANNEL_OUT_BOTTOM_FRONT_CENTER = 0x800000,
        CHANNEL_OUT_BOTTOM_FRONT_RIGHT = 0x1000000,
        CHANNEL_OUT_LOW_FREQUENCY_2 = 0x2000000,
        CHANNEL_OUT_FRONT_WIDE_LEFT = 0x4000000,
        CHANNEL_OUT_FRONT_WIDE_RIGHT = 0x8000000,
        CHANNEL_OUT_MONO = CHANNEL_OUT_FRONT_LEFT,
        CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT),
        CHANNEL_OUT_QUAD = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        CHANNEL_OUT_QUAD_SIDE = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT),
        CHANNEL_OUT_SURROUND = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_BACK_CENTER),
        CHANNEL_OUT_5POINT1 = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY | CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        CHANNEL_OUT_5POINT1_SIDE = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT),
        CHANNEL_OUT_7POINT1_SURROUND = (
                CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT |
                CHANNEL_OUT_LOW_FREQUENCY),
        CHANNEL_OUT_5POINT1POINT2 = (CHANNEL_OUT_5POINT1 |
                CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT),
        CHANNEL_OUT_5POINT1POINT4 = (CHANNEL_OUT_5POINT1 |
                CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_RIGHT |
                CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT),
        CHANNEL_OUT_7POINT1POINT2 = (CHANNEL_OUT_7POINT1_SURROUND |
                CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT),
        CHANNEL_OUT_7POINT1POINT4 = (CHANNEL_OUT_7POINT1_SURROUND |
                CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_RIGHT |
                CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT),
        CHANNEL_OUT_9POINT1POINT4 = (CHANNEL_OUT_7POINT1POINT4
                | CHANNEL_OUT_FRONT_WIDE_LEFT | CHANNEL_OUT_FRONT_WIDE_RIGHT),
        CHANNEL_OUT_9POINT1POINT6 = (CHANNEL_OUT_9POINT1POINT4
                | CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT),
        CHANNEL_OUT_13POINT_360RA = (
                CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT |
                CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_CENTER |
                CHANNEL_OUT_TOP_FRONT_RIGHT |
                CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT |
                CHANNEL_OUT_BOTTOM_FRONT_LEFT | CHANNEL_OUT_BOTTOM_FRONT_CENTER |
                CHANNEL_OUT_BOTTOM_FRONT_RIGHT),
        CHANNEL_OUT_22POINT2 = (CHANNEL_OUT_7POINT1POINT4 |
                CHANNEL_OUT_FRONT_LEFT_OF_CENTER | CHANNEL_OUT_FRONT_RIGHT_OF_CENTER |
                CHANNEL_OUT_BACK_CENTER | CHANNEL_OUT_TOP_CENTER |
                CHANNEL_OUT_TOP_FRONT_CENTER | CHANNEL_OUT_TOP_BACK_CENTER |
                CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT |
                CHANNEL_OUT_BOTTOM_FRONT_LEFT | CHANNEL_OUT_BOTTOM_FRONT_RIGHT |
                CHANNEL_OUT_BOTTOM_FRONT_CENTER |
                CHANNEL_OUT_LOW_FREQUENCY_2),

    } channel_config;



    int buffer_size_in_bytes;

    enum Mode {
        MODE_STATIC = 0,
        MODE_STREAM = 1,
    } mode;

    enum WriteMode {
        WRITE_BLOCKING     = 0,
        WRITE_NON_BLOCKING = 1,
    } write_mode; // not used

    // extra field
    int sdl_samples;
} SDL_Android_AudioTrack_Spec;


typedef struct SDL_Android_AudioTrack SDL_Android_AudioTrack;

int AudioTrack_sampleRate(SDL_Android_AudioTrack *track);
int AudioTrack_audioFormat(SDL_Android_AudioTrack *track);
int AudioTrack_channelCount(SDL_Android_AudioTrack *track);
int AudioTrack_desiredFrames(SDL_Android_AudioTrack *track);


extern SDL_Android_AudioTrack *AudioTrack_Open(SDL_bool isCapture, int sampleRate, int audioFormat, int desiredChannels, int desiredFrames);

extern void AudioTrack_Close( SDL_Android_AudioTrack *track);

extern void AudioTrack_WriteByteBuffer(SDL_Android_AudioTrack *track, jbyteArray buffer);
extern void AudioTrack_WriteShortBuffer(SDL_Android_AudioTrack *track, jshortArray buffer);
extern void AudioTrack_WriteFloatBuffer(SDL_Android_AudioTrack *track, jfloatArray buffer);

#endif /*SDL_audio_track_h_*/