#ifndef FF_clock_c_h_
#define FF_clock_c_h_


#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001


typedef struct VideoState VideoState;

typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

extern double get_clock(Clock *c);

extern void set_clock_at(Clock *c, double pts, int serial, double time);

extern void set_clock(Clock *c, double pts, int serial);

extern void set_clock_speed(Clock *c, double speed);

extern void init_clock(Clock *c, int *queue_serial);

extern void sync_clock_to_slave(Clock *c, Clock *slave);

extern int get_master_sync_type(VideoState *is);

extern double get_master_clock(VideoState *is);

extern void check_external_clock_speed(VideoState *is);

#endif /*FF_clock_c_h_*/