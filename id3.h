#ifndef ID3_DEF
#define ID3_DEF

#include "songinfo.h"

void ID3_Open(const char *filename, SongInfo *info);
int ID3_GetTagSize(char *buff);

#endif
