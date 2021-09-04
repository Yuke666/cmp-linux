#ifndef LASTFM_DEF
#define LASTFM_DEF

#include "songinfo.h"

void LastFM_Init();
void LastFM_Close();
void LastFM_Authenticate(char *username, char *password);
void LastFM_ScrobbleTrack(SongInfo song, time_t startedTime);

#endif