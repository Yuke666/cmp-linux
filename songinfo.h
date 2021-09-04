#ifndef SONG_INFO_DEF
#define SONG_INFO_DEF

typedef struct {
    char *pixels;
    int width;
    int height;
    char channels;
} SongImage;

typedef struct {
    char title[2048];
    char album[2048];
    char year[5];
    int track;
    char contentType[2048];
    char composer[2048];
    char artist[2048];
    SongImage image;
    char filepath[2028];
    int ext;
} SongInfo;

typedef struct {
	SongInfo *songs;
	int numOfSongs;
	SongImage cover;
	char name[2048];
} Album;

#endif