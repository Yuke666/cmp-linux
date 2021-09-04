#include <pulse/pulseaudio.h>
#include <math.h>
#include <stdio.h>
#include "pulse.h"
#include "player.h"
#include <ncurses.h>
#include <string.h>

#define SAMPLES_BEFORE_REALLOC 100000000

static pa_threaded_mainloop *pa_ml;
static pa_mainloop_api *pa_mlapi;
static pa_context *pa_ctx;
static pa_channel_map pa_cmap;
static pa_stream *pa_s;
static pa_cvolume pa_vol;
static pa_buffer_attr bufattr;
static pa_sample_spec ss;
static char *samples = NULL;
// static short blankSamples[SAMPLES_BEFORE_REALLOC];
static int allocatedSampleMemory = 0;
static int latency = 20000;
static int samplesSize = SAMPLES_BEFORE_REALLOC;
static int sampleRate = 44100;
static int sampleoffs = 0;
static int underflows = 0;
static int l_volume, r_volume;

static void PA_SuccessCallback(pa_stream *s, int success, void *data){
    pa_threaded_mainloop_signal(pa_ml, 0);
}

// static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {

//     if(sampleoffs*2 + length > samplesSize){
//         sampleoffs = 0;
//         // if(endCallback != NULL) endCallback();
//     }

//     if(length > samplesSize)
//         length = samplesSize;

//     if(samples){
//         pa_stream_write(s, (short*)&samples[sampleoffs*2], length, NULL, 0LL, PA_SEEK_RELATIVE);
//         sampleoffs += length/2;
//     } else {
//         sampleoffs = 0;
//         pa_stream_write(s, (short*)&blankSamples[0], length, NULL, 0LL, PA_SEEK_RELATIVE);
//     }
// }

static void stream_underflow_cb(pa_stream *s, void *userdata) {
    underflows++;
    if (underflows >= 6 && latency < 2000000) {
        latency = (latency*3)/2;
        bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
        bufattr.tlength = pa_usec_to_bytes(latency,&ss);  
        pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
        underflows = 0;
    }
}

static void pa_state_cb(pa_context *c, void *userdata){
    pa_context_state_t state;
    int *pa_ready = userdata;
    state = pa_context_get_state(c);
    if(state == PA_CONTEXT_TERMINATED) *pa_ready = 2;
    if(state == PA_CONTEXT_READY) *pa_ready = 1;
}

short *Pulse_GetCurrentSamples(int *num){
    if(!samples){
        if(num != NULL) *num = 0;
        return 0;
    }
    if(num != NULL) *num = samplesSize - (sampleoffs*2);
    return (short*)&samples[sampleoffs*2];
}

int Pulse_GetSamplesSize(){
    return samplesSize;
}

int Pulse_GetSampleOffset(){
    return sampleoffs;
}

int Pulse_GetSampleRate(){
    return sampleRate;
}

int Pulse_GetSampleIndex(){
    return samplesSize;
}

void Pulse_ClearSamples(){
    Pulse_StopPlaying();
    if(samples) free(samples);
    samples = NULL;
    samplesSize = 0;
    sampleoffs = 0;
    allocatedSampleMemory = 0;
}

void Pulse_AddSample(char sample){

    samplesSize++;
    if(samplesSize > allocatedSampleMemory){
        allocatedSampleMemory += SAMPLES_BEFORE_REALLOC;
        char *moreSamples = NULL;
        moreSamples = (char*)realloc(samples, sizeof(char)*allocatedSampleMemory);
        
        if(moreSamples)
            samples=moreSamples;
        else {
            endwin();
            printf("Error while allocating memory for the song.\n");
            exit(0);
        }
    }
    
    if(samples) samples[samplesSize-1] = sample;
    sampleoffs = 0;
}

void Pulse_SetSampleIndex(int index){
    samplesSize = index;
}

int Pulse_GetWritableSize(){
    pa_threaded_mainloop_lock(pa_ml);
    int ret = (int)pa_stream_writable_size(pa_s);
    pa_threaded_mainloop_unlock(pa_ml);
    return ret;
}

void Pulse_Write(short *samples, int len){
    if(len < Pulse_GetWritableSize() || !samples) return;
    pa_threaded_mainloop_lock(pa_ml);
    pa_stream_write(pa_s, samples, len, NULL, 0LL, PA_SEEK_RELATIVE);
    pa_threaded_mainloop_unlock(pa_ml);
    sampleoffs += len/2;
}

void Pulse_Seek(float amount){
    Pulse_Pause();
    if(amount < 0) amount = 0;
    if(amount > 1){
        Player_OnSongEnd();
        amount = 0;
    }
    sampleoffs = (samplesSize/2) * amount;
    Pulse_Unpause();
}

float Pulse_GetSeek(){
    return round(((float)(sampleoffs*2) / (float)samplesSize)/0.01) * 0.01;
}

int Pulse_GetDurationInSecs(){
    return (samplesSize/4)/sampleRate;
}

static void WaitUnlock(pa_operation *operation){
    while(pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(pa_ml);

    if(operation) pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(pa_ml);
} 

void Pulse_Unpause(){
    Player_Play();
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_stream_cork(pa_s, 0, PA_SuccessCallback, NULL);
    WaitUnlock(o);
}

void Pulse_Pause(){
    Player_Stop();
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_stream_cork(pa_s, 1, PA_SuccessCallback, NULL);
    WaitUnlock(o);
}

void Pulse_StopPlaying(){
    sampleoffs = 0;
    Pulse_Pause();
}

void Pulse_StartPlaying(){
    sampleoffs = 0;
    Pulse_Unpause();
}

static void PA_SinkInfoCB(pa_context *c, const pa_sink_input_info *i, int eol, void *data){
    if(i == NULL) return;
    pa_cvolume volume = i->volume;
    r_volume = pa_cvolume_get_position(&volume, &pa_cmap, PA_CHANNEL_POSITION_FRONT_RIGHT);
    l_volume = pa_cvolume_get_position(&volume, &pa_cmap, PA_CHANNEL_POSITION_FRONT_LEFT);
    pa_threaded_mainloop_signal(pa_ml, 0);
}

void Pulse_SetVolume(float vol){

    vol = round(vol/0.1)*0.1;
    vol = ((vol * 100) + 50) / 100;
    vol = (float)(vol / 100.0) * PA_VOLUME_NORM;

    pa_threaded_mainloop_lock(pa_ml);
    pa_volume_t v = vol;
    pa_cvolume_set(&pa_vol, 2, (pa_volume_t) v);

    int index = pa_stream_get_index(pa_s);
    pa_context_set_sink_input_volume(pa_ctx, index, &pa_vol, NULL, NULL);

    pa_threaded_mainloop_unlock(pa_ml);
}

int Pulse_GetVolume(){
    pa_threaded_mainloop_lock(pa_ml);

    int index = pa_stream_get_index(pa_s);

    pa_operation *o = pa_context_get_sink_input_info(pa_ctx, index, PA_SinkInfoCB, NULL);
    WaitUnlock(o);

    float avg = (float)(l_volume + r_volume) / 2;

    return (avg * 100) / PA_VOLUME_NORM;
}

void Pulse_Close(){

    pa_threaded_mainloop_lock(pa_ml);

    if(pa_s){
        pa_stream_flush(pa_s, PA_SuccessCallback, NULL);
        pa_stream_disconnect(pa_s);
        pa_stream_unref(pa_s);
    }

    if(pa_ctx){
        pa_context_disconnect(pa_ctx);
        pa_context_unref(pa_ctx);
    }

    pa_threaded_mainloop_unlock(pa_ml);

    if(pa_ml){
        pa_threaded_mainloop_stop(pa_ml);
        pa_threaded_mainloop_free(pa_ml);
    }

    if(samples) free(samples);
    samples = NULL;
}

int Pulse_Open(){
    int r;
    int pa_ready = 0;
    pa_ml = pa_threaded_mainloop_new();
    pa_threaded_mainloop_start(pa_ml);
    pa_mlapi = pa_threaded_mainloop_get_api(pa_ml);
    pa_threaded_mainloop_lock(pa_ml);

    pa_proplist *proplist = pa_proplist_new();
    pa_ctx = pa_context_new_with_proplist(pa_mlapi, "cmp", proplist);
    pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFAIL, NULL);
    pa_proplist_free(proplist);

    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);
    pa_threaded_mainloop_unlock(pa_ml);

    while(1) {
        if(pa_ready == 2) return 0;
        if(pa_ready == 1) break;
        pa_threaded_mainloop_wait(pa_ml);
    }

    pa_threaded_mainloop_lock(pa_ml);

    ss.rate = sampleRate;
    ss.channels = 2;
    ss.format = PA_SAMPLE_S16LE;
    
    pa_channel_map_init_auto(&pa_cmap, ss.channels, PA_CHANNEL_MAP_ALSA);

    pa_s = pa_stream_new(pa_ctx, "Playback", &ss, &pa_cmap);

    if(!pa_s){
        printf("pa_stream_new failed.\n");
    }

    // pa_stream_set_write_callback(pa_s, stream_request_cb, NULL);
    pa_stream_set_underflow_callback(pa_s, stream_underflow_cb, NULL);
    bufattr.fragsize = (uint32_t)-1;
    bufattr.maxlength = pa_usec_to_bytes(latency, &ss);
    bufattr.minreq = pa_usec_to_bytes(0, &ss);
    bufattr.prebuf =  (uint32_t)-1;
    bufattr.tlength = pa_usec_to_bytes(latency, &ss);

    r = pa_stream_connect_playback(pa_s, NULL, &bufattr,
                                 PA_STREAM_INTERPOLATE_TIMING
                                 |PA_STREAM_ADJUST_LATENCY
                                 |PA_STREAM_AUTO_TIMING_UPDATE
                                 |PA_STREAM_START_CORKED , NULL, NULL);
    if (r < 0) {
        r = pa_stream_connect_playback(pa_s, NULL, &bufattr,
                                       PA_STREAM_INTERPOLATE_TIMING|
                                       PA_STREAM_AUTO_TIMING_UPDATE|
                                       PA_STREAM_START_CORKED , NULL, NULL);
    }
    if (r < 0) {
        printf("pa_stream_connect_playback failed\n");
        return 0;
    }

    pa_threaded_mainloop_unlock(pa_ml);

    while (pa_stream_get_state(pa_s) != PA_STREAM_READY)
        pa_threaded_mainloop_wait(pa_ml);
        
    Pulse_StopPlaying();

    return 1;
}