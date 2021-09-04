#ifndef FLAC_DEF
#define FLAC_DEF

#include "songinfo.h"
void Flac_GetInfo(const char *filename, SongInfo *info);
void Flac_Play(const char *filename);

#endif