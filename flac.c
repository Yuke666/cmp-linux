#include <FLAC/all.h>
#include <string.h>
#include <stdlib.h>
#include "flac.h"
#include "pulse.h"
#include "ui.h"

static int sampleRate;
static int totalSamples;
static int channels;
static int bps;

static FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, 
	const FLAC__int32 * const buffer[], void *clientData){


	if(totalSamples == 0 || channels != 2 || bps != 16 || frame->header.channels != channels || 
		buffer[0] == NULL || buffer[1] == NULL) {
		
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	int k;
	for(k = 0; k < frame->header.blocksize; k++) {
			
		FLAC__int16 sample = (FLAC__int16)buffer[0][k]; //  left channel 
		Pulse_AddSample(((sample >> 0) & 0xFF));
		Pulse_AddSample(((sample >> 8) & 0xFF));

		sample = (FLAC__int16)buffer[1][k]; // right channel
		Pulse_AddSample(((sample >> 0) & 0xFF));
		Pulse_AddSample(((sample >> 8) & 0xFF));
	}


	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void errorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data){
	printf("%s\n",FLAC__StreamDecoderErrorStatusString[status]);
}

static void metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data){
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		totalSamples = metadata->data.stream_info.total_samples;
		sampleRate = metadata->data.stream_info.sample_rate;
		channels = metadata->data.stream_info.channels;
		bps = metadata->data.stream_info.bits_per_sample;

	}
}

void Flac_Play(const char *filename){
	
	Pulse_SetSampleIndex(0);
	Pulse_StopPlaying();
	Pulse_ClearSamples();

	sampleRate = 0;
	totalSamples = 0;
	channels = 0;
	bps = 0;


	FLAC__StreamDecoder *decoder;
	FLAC__StreamDecoderInitStatus status;
	decoder = FLAC__stream_decoder_new();

	FLAC__stream_decoder_set_md5_checking(decoder, true);

	status = FLAC__stream_decoder_init_file(decoder, filename, writeCallback, metadataCallback, errorCallback, NULL);
	if(status != FLAC__STREAM_DECODER_INIT_STATUS_OK){
		FLAC__stream_decoder_delete(decoder);
		return;
	}

	if(FLAC__stream_decoder_process_until_end_of_stream(decoder)){
		Pulse_StartPlaying();
	}
	
	FLAC__stream_decoder_delete(decoder);
}


void Flac_GetInfo(const char *filename, SongInfo *info){
	

    FLAC__StreamMetadata *streaminfo;
    if(!FLAC__metadata_get_tags(filename, &streaminfo))
    	return;

    if(streaminfo->type == FLAC__METADATA_TYPE_VORBIS_COMMENT){
        
        int i;
        for(i = 0; i < streaminfo->data.vorbis_comment.num_comments; i++){

        	char *comment = (char *)streaminfo->data.vorbis_comment.comments[i].entry;
        	
        	char *type = strtok(comment, "=");
        	char *str = 0;
        	
        	if(type != NULL){
        		str = strtok(NULL, "=");
        		if(str == NULL) continue;
	        	if(strcmp(type, "TITLE") == 0) strcpy(info->title, str);
	        	if(strcmp(type, "ARTIST") == 0) strcpy(info->artist, str);
	        	if(strcmp(type, "ALBUM") == 0) strcpy(info->album, str);
	        	if(strcmp(type, "COMPOSER") == 0) strcpy(info->composer, str);
	        	if(strcmp(type, "GENRE") == 0) strcpy(info->contentType, str);
	        	if(strcmp(type, "TRACKNUMBER") == 0) info->track = atoi(str);
	        } else {
	        	continue;
	        }
        }
    }


    FLAC__metadata_object_delete(streaminfo);

    info->image.pixels = NULL;
 //    if(!FLAC__metadata_get_picture(filename, &streaminfo, -1, NULL, NULL, (unsigned)-1, (unsigned)-1, (unsigned)-1, (unsigned)-1)){
 //    	return;
 //    } 

 //    if(streaminfo->type == FLAC__METADATA_TYPE_PICTURE){
	// }

 //    FLAC__metadata_object_delete(streaminfo);
}