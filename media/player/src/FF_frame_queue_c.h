#ifndef FF_frame_queue_c_h_
#define FF_frame_queue_c_h_

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/rational.h"

#include "SDL_mutex.h"
#include "SDL_thread.h"

#include "FF_packet_queue_c.h"


#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;


extern int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);

extern void frame_queue_destory(FrameQueue *f);

extern void frame_queue_signal(FrameQueue *f);

extern Frame *frame_queue_peek(FrameQueue *f);

extern Frame *frame_queue_peek_next(FrameQueue *f);

extern Frame *frame_queue_peek_last(FrameQueue *f);

extern Frame *frame_queue_peek_writable(FrameQueue *f);

extern Frame *frame_queue_peek_readable(FrameQueue *f);

extern void frame_queue_push(FrameQueue *f);

extern void frame_queue_next(FrameQueue *f);

extern int frame_queue_nb_remaining(FrameQueue *f);

extern int64_t frame_queue_last_pos(FrameQueue *f);



#endif /*FF_frame_queue_c_h_*/