#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include "utils.h"
#include "visualizer_win.h"
#include "config.h"
#include "pulse.h"
#include "mp3.h"
#include "flac.h"
#include "lastfm.h"
#include "ui.h"
#include "id3.h"
#include "x11.h"

static SongInfo playingSong;
static time_t playingSongStartTime;
static void (*endSongCallback)() = NULL;
static char currentlyPlaying[1024];
static int playing = 0;
static pthread_t thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t playingCond;

void Player_SetPlayingSong(SongInfo p){
	playingSong = p;
	time(&playingSongStartTime);
}

void Player_ScrobblePlayingSong(){
    time_t currTime;
    time(&currTime);

    if((int)playingSongStartTime != 0 && Pulse_GetDurationInSecs() > 30 &&
        (((int)currTime - (int)playingSongStartTime > Pulse_GetDurationInSecs()/2) ||
        ((int)currTime - (int)playingSongStartTime > 60*4))){

        LastFM_ScrobbleTrack(playingSong, playingSongStartTime);
    }
}

void Player_SetEndSongCallback(void *c){
	endSongCallback = c;
}

void Player_OnSongEnd(){
	playing = 0;
	if(endSongCallback) endSongCallback();
}

void Player_SetPlayingSongStr(const char *str){
	strcpy(currentlyPlaying, str);
}

char *Player_GetPlayingSongStr(){
	return currentlyPlaying;
}

void *Player_MainLoop(void *args){
	while(1){

		pthread_mutex_lock(&mutex);
		if(!playing) {
			pthread_cond_wait(&playingCond, &mutex);
			pthread_mutex_unlock(&mutex);
			continue;
		}

		int writeSize = Pulse_GetWritableSize();

		if(writeSize+(Pulse_GetSampleOffset()*2) >= Pulse_GetSamplesSize() && Pulse_GetSamplesSize() > 0){

			playing = 0;
			Player_OnSongEnd();
			pthread_mutex_unlock(&mutex);
			continue;

		} else if(writeSize > 0){
			short *samples = Pulse_GetCurrentSamples(NULL);
			Pulse_Write(samples, writeSize);
		}

		pthread_mutex_unlock(&mutex);
		usleep(10000);
	}

	pthread_exit(NULL);
}

void Player_Init(){
	pthread_create(&thread, NULL, &Player_MainLoop, NULL);
}

void Player_Play(){
	if(playing) return;
	playing = 1;
	pthread_cond_broadcast(&playingCond);
}

void Player_Stop(){
	playing = 0;
}
