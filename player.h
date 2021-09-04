#ifndef PLAYER_DEF
#define PLAYER_DEF

#include "songinfo.h"
void Player_SetEndSongCallback();
void Player_Init();
void Player_SetPlayingSongStr(const char *str);
char *Player_GetPlayingSongStr();
void Player_ScrobblePlayingSong();
void Player_OnSongEnd();
void Player_SetPlayingSong(SongInfo p);
void Player_Stop();
void Player_Play();

#endif