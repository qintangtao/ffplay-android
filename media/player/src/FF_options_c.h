#ifndef FF_options_c_h_
#define FF_options_c_h_

#define OPTION_OFFSET(x) offsetof(FFPlayer, x)
#define OPTION_INT(default__, min__, max__) \
    .type = AV_OPT_TYPE_INT, \
    { .i64 = default__ }, \
    .min = min__, \
    .max = max__, \
    .flags = AV_OPT_FLAG_DECODING_PARAM
#define OPTION_INT64(default__, min__, max__) \
    .type = AV_OPT_TYPE_INT64, \
    { .i64 = default__ }, \
    .min = min__, \
    .max = max__, \
    .flags = AV_OPT_FLAG_DECODING_PARAM
#define OPTION_DOUBLE(default__, min__, max__) \
    .type = AV_OPT_TYPE_DOUBLE, \
    { .dbl = default__ }, \
    .min = min__, \
    .max = max__, \
    .flags = AV_OPT_FLAG_DECODING_PARAM
#define OPTION_CONST(default__) \
    .type = AV_OPT_TYPE_CONST, \
    { .i64 = default__ }, \
    .min = INT_MIN, \
    .max = INT_MAX, \
    .flags = AV_OPT_FLAG_DECODING_PARAM

#define OPTION_STR(default__) \
    .type = AV_OPT_TYPE_STRING, \
    { .str = default__ }, \
    .min = 0, \
    .max = 0, \
    .flags = AV_OPT_FLAG_DECODING_PARAM


#define OPTION_BOOL(default__) \
    .type = AV_OPT_TYPE_BOOL, \
    { .i64 = default__ }, \
    .min = 0, \
    .max = 1, \
    .flags = AV_OPT_FLAG_DECODING_PARAM

    
static const AVOption FF_options[] = {
    { "acodec",                            "force audio decoder",
        OPTION_OFFSET(audio_codec_name),        OPTION_STR(NULL) },
    { "scodec",                            "force subtitle decoder",
        OPTION_OFFSET(subtitle_codec_name),     OPTION_STR(NULL) },
    { "vcodec",                            "force video decoder",
        OPTION_OFFSET(video_codec_name),        OPTION_STR(NULL) },

    { "drp",                                "let decoder reorder pts 0=off 1=on -1=auto",
        OPTION_OFFSET(decoder_reorder_pts),     OPTION_INT(-1, -1, 1) },
    { "volume",                             "set startup volume 0=min 100=max",
        OPTION_OFFSET(startup_volume),          OPTION_INT(100, 0, 100) },

    { "sync",                               "set audio-video sync. type (type=audio/video/ext)",
        OPTION_OFFSET(startup_volume),          OPTION_INT(AV_SYNC_AUDIO_MASTER, INT_MIN, INT_MAX),
        .unit = "sync" },
    { "audio",                              "", 0, OPTION_CONST(AV_SYNC_AUDIO_MASTER), .unit = "sync" },
    { "video",                              "", 0, OPTION_CONST(AV_SYNC_VIDEO_MASTER), .unit = "sync" },
    { "ext",                                "", 0, OPTION_CONST(AV_SYNC_EXTERNAL_CLOCK), .unit = "sync" },

    { "lowres",                             "",
        OPTION_OFFSET(lowres),                  OPTION_INT(100, INT_MIN, INT_MAX) },
    { "fast",                               "non spec compliant optimizations",
        OPTION_OFFSET(fast),                    OPTION_BOOL(0) },
    { "genpts",                             "generate pts",
        OPTION_OFFSET(genpts),                  OPTION_BOOL(0) },

    { "find_stream_info",                   "read and decode the streams to fill missing information with heuristics",
        OPTION_OFFSET(find_stream_info),        OPTION_BOOL(0) },           
    { "bytes",                              "seek by bytes 0=off 1=on -1=auto",
        OPTION_OFFSET(seek_by_bytes),           OPTION_INT(-1, -1, 1) },
    { "stats",                              "show status",
        OPTION_OFFSET(show_status),             OPTION_BOOL(0) }, 
    { "loop",                               "set number of times the playback shall be looped",
        OPTION_OFFSET(loop),                    OPTION_INT(1, 1, INT_MAX) },
    { "framedrop",                           "drop frames when cpu is too slow",
        OPTION_OFFSET(framedrop),               OPTION_INT(-1, -1, 1) },    
    { "autoexit",                           "exit at the end",
        OPTION_OFFSET(autoexit),                OPTION_BOOL(1) }, 
    { "infbuf",                             "don't limit the input buffer size (useful with realtime streams)",
        OPTION_OFFSET(infinite_buffer),         OPTION_INT(-1, -1, 1) }, 

    { "showmode",                           "select show mode (0 = video, 1 = waves, 2 = RDFT)",
        OPTION_OFFSET(show_mode),               OPTION_INT(SHOW_MODE_NONE, INT_MIN, INT_MAX),
        .unit = "showmode" },
    { "video",                              "", 0, OPTION_CONST(SHOW_MODE_VIDEO), .unit = "showmode" },
    { "waves",                              "", 0, OPTION_CONST(SHOW_MODE_WAVES), .unit = "showmode" },
    { "RDFT",                               "", 0, OPTION_CONST(SHOW_MODE_RDFT), .unit = "showmode" },

    { "an",                                 "disable audio",
        OPTION_OFFSET(audio_disable),           OPTION_BOOL(0) }, 
    { "vn",                                 "disable video",
        OPTION_OFFSET(video_disable),           OPTION_BOOL(0) }, 
    { "sn",                                 "disable subtitling",
        OPTION_OFFSET(subtitle_disable),        OPTION_BOOL(0) }, 


	{ NULL }
};


#undef OPTION_BOOL
#undef OPTION_STR
#undef OPTION_CONST
#undef OPTION_INT
#undef OPTION_OFFSET

#endif /*FF_options_c_h_*/