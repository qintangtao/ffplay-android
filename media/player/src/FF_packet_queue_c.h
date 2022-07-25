#ifndef FF_packet_queue_c_h_
#define FF_packet_queue_c_h_

#include <stdint.h>

#include "libavcodec/packet.h"
#include "libavutil/fifo.h"

#include "SDL_mutex.h"
#include "SDL_thread.h"


typedef struct MyAVPacketList {
    AVPacket *pkt;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    AVFifoBuffer *pkt_list;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;


extern int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);

extern int packet_queue_put(PacketQueue *q, AVPacket *pkt);

extern int packet_queue_put_nullpacket(PacketQueue *q, AVPacket *pkt, int stream_index);

extern int packet_queue_init(PacketQueue *q);

extern void packet_queue_flush(PacketQueue *q);

extern void packet_queue_destroy(PacketQueue *q);

extern void packet_queue_abort(PacketQueue *q);

extern void packet_queue_start(PacketQueue *q);

extern int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

 
#endif /*FF_packet_queue_c_h_*/