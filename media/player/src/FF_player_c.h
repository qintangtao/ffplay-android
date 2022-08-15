#ifndef FF_player_c_h_
#define FF_player_c_h_

#include <stdint.h>

#include "libavutil/avutil.h"

#include "FF_common_c.h"

#define AV_NOSYNC_THRESHOLD 10.0


typedef uint32_t SDL_AudioDeviceID;
typedef struct VideoState VideoState;
typedef struct AVDictionary AVDictionary;


typedef struct FFPlayer {

	/* ffplay context */
    VideoState *is;

    /* audio */
	SDL_AudioDeviceID audio_dev;

	int64_t audio_callback_time;

	int64_t start_time;// = AV_NOPTS_VALUE;
	int64_t duration;// = AV_NOPTS_VALUE;

	//options
	AVDictionary *format_opts;
	AVDictionary *codec_opts;
	AVDictionary *sws_dict;
	AVDictionary *swr_opts;

	char *audio_codec_name;
	char *subtitle_codec_name;
	char *video_codec_name;

	int decoder_reorder_pts;// = -1;
	int startup_volume;// = 100;
	int av_sync_type;// = AV_SYNC_AUDIO_MASTER;

	int lowres;// = 0;
	int fast;// = 0;
	int genpts;// = 0;

	int find_stream_info;// = 1;
	int seek_by_bytes;// = -1;
	int show_status;// = -1;
	int loop;// = 1;
	int framedrop;// = -1;
	int autoexit; // = 1
	int infinite_buffer;// = -1;

	enum ShowMode show_mode; // = SHOW_MODE_NONE;
	int audio_disable;
	int video_disable;
	int subtitle_disable;
	const char* wanted_stream_spec[AVMEDIA_TYPE_NB];

} FFPlayer;

void FF_reset_internal(FFPlayer *ffp);

#endif /*FF_player_c_h_*/



