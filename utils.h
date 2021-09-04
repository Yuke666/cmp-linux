#ifndef UTILS_STR_DEF
#define UTILS_STR_DEF
#include "songinfo.h"
#include "ui.h"

enum {
    EXTENSION_UNSUPPORTED,
    EXTENSION_MP3,
    EXTENSION_FLAC,
    EXTENSION_JPEG,
    EXTENSION_PNG,
};


#define FORMAT_STR_SEPERATOR 255

void Utils_FormatStr(WINDOW *win, int len, const char *format, ...);
int Utils_LoadPNG(FILE *fp, SongImage *img);
int Utils_LoadJPEG(FILE *fp, SongImage *image);
int Utils_GetExt(char *str);
int Utils_QSortStr(const void *one, const void *two);

#endif