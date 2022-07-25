#ifndef FF_audio_c_h_
#define FF_audio_c_h_

#include <stdint.h>

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10


typedef struct FFPlayer FFPlayer;
typedef struct AudioParams AudioParams;

int audio_open(FFPlayer *ffp, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, AudioParams *audio_hw_params);


#endif /*FF_audio_c_h_*/