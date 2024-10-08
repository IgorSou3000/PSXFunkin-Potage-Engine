/*
psxavenc: MDEC video + SPU/XA-ADPCM audio encoder frontend

Copyright (c) 2019, 2020 Adrian "asie" Siekierka
Copyright (c) 2019 Ben "GreaseMonkey" Russell

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/avdct.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "libpsxav/libpsxav.h"

#define NUM_FORMATS 10
#define NUM_FRAME_FORMATS 3

typedef enum {
	FORMAT_XA,
	FORMAT_XACD,
	FORMAT_SPU,
	FORMAT_SPUI,
	FORMAT_VAG,
	FORMAT_VAGI,
	FORMAT_STR,
	FORMAT_STRCD,
	FORMAT_STRV,
	FORMAT_SBS,
	FORMAT_INVALID
} format_t;

typedef enum {
	FRAME_FORMAT_V2,
	FRAME_FORMAT_V3,
	FRAME_FORMAT_V3B,
	FRAME_FORMAT_INVALID
} frame_format_t;

typedef struct {
	int frame_index;
	int frame_data_offset;
	int frame_max_size;
	int frame_block_base_overflow;
	int frame_block_overflow_num;
	int frame_block_overflow_den;
	int block_type;
	int16_t last_dc_values[3];
	uint16_t bits_value;
	int bits_left;
	uint8_t *frame_output;
	int bytes_used;
	int blocks_used;
	int uncomp_hwords_used;
	int quant_scale;
	int quant_scale_sum;

	uint32_t *ac_huffman_map;
	uint32_t *dc_huffman_map;
	int16_t *coeff_clamp_map;
	int16_t *delta_clamp_map;
	int16_t *dct_block_lists[6];
	AVDCT *dct_context;
} vid_encoder_state_t;

typedef struct {
	int video_frame_dst_size;
	int audio_stream_index;
	int video_stream_index;
	AVFormatContext* format;
	AVStream* audio_stream;
	AVStream* video_stream;
	AVCodecContext* audio_codec_context;
	AVCodecContext* video_codec_context;
	struct SwrContext* resampler;
	struct SwsContext* scaler;
	AVFrame* frame;

	int sample_count_mul;

	double video_next_pts;
} av_decoder_state_t;

typedef struct {
	bool quiet;
	bool show_progress;

	format_t format;
	frame_format_t frame_format;

	int channels;
	int cd_speed; // 1 or 2
	int frequency; // 18900 or 37800 Hz
	int bits_per_sample; // 4 or 8
	int file_number; // 00-FF
	int channel_number; // 00-1F
	int interleave;
	int alignment;
	bool loop;

	int video_width;
	int video_height;
	int video_fps_num; // FPS numerator
	int video_fps_den; // FPS denominator
	bool ignore_aspect_ratio;
	bool trailing_audio;

	char *swresample_options;
	char *swscale_options;

	int16_t *audio_samples;
	int audio_sample_count;
	uint8_t *video_frames;
	int video_frame_count;

	av_decoder_state_t decoder_state_av;
	vid_encoder_state_t state_vid;
	bool end_of_input;

	time_t start_time;
	time_t last_progress_update;
} settings_t;

// cdrom.c
void init_sector_buffer_video(uint8_t *buffer, settings_t *settings);
void calculate_edc_data(uint8_t *buffer);

// decoding.c
bool open_av_data(const char *filename, settings_t *settings, bool use_audio, bool use_video, bool audio_required, bool video_required);
bool poll_av_data(settings_t *settings);
bool ensure_av_data(settings_t *settings, int needed_audio_samples, int needed_video_frames);
void retire_av_data(settings_t *settings, int retired_audio_samples, int retired_video_frames);
void close_av_data(settings_t *settings);

// filefmt.c
void encode_file_spu(settings_t *settings, FILE *output);
void encode_file_spu_interleaved(settings_t *settings, FILE *output);
void encode_file_xa(settings_t *settings, FILE *output);
void encode_file_str(settings_t *settings, FILE *output);
void encode_file_sbs(settings_t *settings, FILE *output);

// mdec.c
bool init_encoder_state(settings_t *settings);
void destroy_encoder_state(settings_t *settings);
void encode_frame_bs(uint8_t *video_frame, settings_t *settings);
void encode_sector_str(uint8_t *video_frames, uint8_t *output, settings_t *settings);
