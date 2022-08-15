#ifndef FF_decoder_c_h_
#define FF_decoder_c_h_

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/rational.h"

#include "SDL_mutex.h"
#include "SDL_thread.h"

#include "../FF_player_c.h"
#include "../FF_packet_queue_c.h"
#include "../FF_frame_queue_c.h"

typedef struct Decoder {
    AVPacket *pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
} Decoder;


extern int decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);

extern int decoder_start(Decoder *d, int (*fn)(void *), const char *thread_name, void* arg);

extern void decoder_abort(Decoder *d, FrameQueue *fq);

extern void decoder_destroy(Decoder *d);

extern int decoder_decode_frame(FFPlayer *ffp, Decoder *d, AVFrame *frame, AVSubtitle *sub);

extern int audio_thread(void *arg);

extern int video_thread(void *arg);

extern int subtitle_thread(void *arg);

#endif /*FF_decoder_c_h_*/


