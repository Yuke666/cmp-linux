#include <mad.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "pulse.h"

#define MAX_CHARS_PER_READ 1000000

struct buffer {
	unsigned char *start;
	unsigned long length;
	FILE *fp;
};

static int hitEOF = 0;
char buffer[MAX_CHARS_PER_READ] = "";

static int fillBuffer(FILE *fp, struct mad_stream *stream){

	int remaining = 0;

	if(stream->next_frame != NULL){
		remaining = stream->bufend - stream->next_frame;
		memmove(buffer, stream->next_frame, remaining);
	}

	int num = fread(&buffer[remaining], sizeof(char), MAX_CHARS_PER_READ - remaining , fp);

	if(num == 0){
		if(!hitEOF){
			memset(&buffer[remaining], 0, MAD_BUFFER_GUARD);
			remaining+=MAD_BUFFER_GUARD;
			hitEOF = 1;
		} else {
			return 0;
		}
	}

	mad_stream_buffer(stream, (const unsigned char*)buffer, num+remaining);

	stream->error = 0;

	return 1;
}

static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame){
	// struct buffer *buffer = data;
	// fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	//   stream->error, mad_stream_errorstr(stream),
	//   (unsigned int)(stream->this_frame - buffer->start));
	return MAD_FLOW_CONTINUE;
}

static inline int scale(mad_fixed_t sample)
{
	sample += 1L << (MAD_F_FRACBITS - 16);
	if (sample >= MAD_F_ONE) {
		sample = MAD_F_ONE - 1;
	} else if (sample < -MAD_F_ONE) {
		sample = -MAD_F_ONE;
	}
	return sample >> (MAD_F_FRACBITS - 15);
}

static enum mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm){
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	nchannels = pcm->channels;
	nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];

	while (nsamples--) {
		signed int sample;

		sample = scale(*left_ch++);
		Pulse_AddSample(((sample >> 0) & 0xFF));
		Pulse_AddSample(((sample >> 8) & 0xFF));

		if (nchannels == 2) {
			sample = scale(*right_ch++);
			Pulse_AddSample(((sample >> 0) & 0xFF));
			Pulse_AddSample(((sample >> 8) & 0xFF));
		}
	}

  return MAD_FLOW_CONTINUE;
}

static enum mad_flow input(void *data, struct mad_stream *stream){
	struct buffer *buffer = data;
	if(!buffer->length) return MAD_FLOW_STOP;

	fillBuffer(buffer->fp , stream);
	if(feof(buffer->fp)) buffer->length = 0;

	return MAD_FLOW_CONTINUE;
}

static enum mad_flow header(void *data, struct mad_header const *header){

	// Pulse_SetSampleRate(header->samplerate);
	// Pulse_SetChannels(MAD_NCHANNELS(header));

	return MAD_FLOW_CONTINUE;
}

static void decode(FILE *fp){
	if(fp == NULL) return;

	struct buffer buffer;
	struct mad_decoder decoder;

	buffer.length = MAX_CHARS_PER_READ;
	buffer.fp = fp;

	mad_decoder_init(&decoder, &buffer, input, header, 0, output, error, 0  );
	mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);
}

int MP3_Play(char *filename){
	
	Pulse_StopPlaying();
	Pulse_ClearSamples();

	hitEOF = 0;
	buffer[0] = 0;

	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) return 0;

	decode(fp);
	fclose(fp);

	Pulse_StartPlaying();

	return 1;
}