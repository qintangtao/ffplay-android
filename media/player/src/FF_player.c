#include "FF_player_c.h"
#include "FF_stream_c.h"
#include "FF_options_c.h"


void FF_reset_internal(FFPlayer *ffp)
{

	ffp->start_time = AV_NOPTS_VALUE;
	ffp->duration = AV_NOPTS_VALUE;

	// options
	ffp->decoder_reorder_pts 	= -1;
	ffp->startup_volume 		= 100;
	ffp->av_sync_type 			= AV_SYNC_AUDIO_MASTER;


	ffp->lowres = 0;
	ffp->fast 	= 0;
	ffp->genpts = 0;

	ffp->find_stream_info 	= 1;
	ffp->seek_by_bytes 		= -1;
	ffp->show_status 		= -1;
	ffp->loop 				= 1;
	ffp->framedrop 			= -1;
	ffp->autoexit			= 1;
	ffp->infinite_buffer 	= -1;

	ffp->show_mode = SHOW_MODE_NONE;
	ffp->audio_disable 		= 0;
	ffp->video_disable 		= 0;
	ffp->subtitle_disable 	= 0;
    memset(ffp->wanted_stream_spec, 0, sizeof(ffp->wanted_stream_spec));
}