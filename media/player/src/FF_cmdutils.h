#ifndef FF_cmdutils_c_h_
#define FF_cmdutils_c_h_

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

extern void print_error(const char *filename, int err);

extern int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);

extern AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, const AVCodec *codec);

extern AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts);

#endif /*FF_cmdutils_c_h_*/