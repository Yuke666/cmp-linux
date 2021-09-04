
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "utils.h"
#include "visualizer_win.h"
#include "config.h"
#include "pulse.h"
#include "mp3.h"
#include "flac.h"
#include "player.h"
#include "ui.h"
#include "id3.h"
#include "x11.h"

enum {
	ARTIST_SELECT_WINODW = 0,
	TRACK_SELECT_WINODW,
};

static int inWindow = ARTIST_SELECT_WINODW;
static int artistIndex = 0;
static int x, y, width, height, newWidth, maxArtistLen;
static int selectWindowOffsetTop = 1;
static int selectWindowOffsetBottom = 1;
static int titleBarHeight = 1;
static int playingSongIndex = 0;
static int hasChanged = 1;
static int artistSelectBarHidden = 0;

static struct artist {
	SongInfo *songs;
	int numOfSongs;
	char name[1024];
} *artists = NULL;

static struct {
	int scrollOffset;
	int selectedLine;
	int numOfItems;
} selectWindows[2];

void LibraryWin_FreeMemory(){
	if(artists) {
		int k;
		for(k = 0; k < artistIndex; k++){
			if(artists[k].songs){
				int j;
				for(j = 0; j < artists[k].numOfSongs; j++)
					if(artists[k].songs[j].image.pixels){
						free(artists[k].songs[j].image.pixels);
						artists[k].songs[j].image.pixels = NULL;
					}

				free(artists[k].songs);
				artists[k].songs = NULL;
			}
		}
	
		free(artists);
	}
	artistIndex = 0;
	artists = NULL;
}

static SongImage LoadDirImage(char *path){
    int k;
    for(k = strlen(path); k > 0; k--)
    	if(path[k] == '/') { path[k] = 0; break; }

	SongImage image;
	image.pixels = NULL;

    DIR *dir;
    struct dirent *dp;

    dir = opendir(path);
    
    if(dir == NULL) return image;
    
    while((dp = readdir(dir)) != NULL){

	    struct stat s;
	    char temp[FILENAME_MAX];
		strcpy(temp, path);
		sprintf(temp, "%s/%s", temp, dp->d_name);

	    stat(temp, &s);

	    if((s.st_mode & S_IFMT) != S_IFDIR){

	    	int fExt = Utils_GetExt(dp->d_name);
	        if(fExt == EXTENSION_JPEG || fExt == EXTENSION_PNG){
	            FILE *tempFile = fopen(temp, "rb");

	            if(!tempFile) continue;

	            if(fExt == EXTENSION_JPEG)
	                if(!Utils_LoadJPEG(tempFile, &image))
		                Utils_LoadPNG(tempFile, &image);

	            if(fExt == EXTENSION_PNG)
	                if(!Utils_LoadPNG(tempFile, &image))
	                	Utils_LoadJPEG(tempFile, &image);

	            if(tempFile) fclose(tempFile);
                closedir(dir);

	            return image;
	        }			        
		}
    }

    closedir(dir);
	return image;
}


static int SortSongInfo(const void *one, const void *two){
    char *artist1 = ((SongInfo *)one)->artist;
    char *artist2 = ((SongInfo *)two)->artist;
    int res = Utils_QSortStr(artist1, artist2);
    if(res != 0) return res;
    char *album1 = ((SongInfo *)one)->album;
    char *album2 = ((SongInfo *)two)->album;
    res = Utils_QSortStr(album1, album2);
    if(res != 0) return res;
    int track1 = ((SongInfo *)one)->track;
    int track2 = ((SongInfo *)two)->track;
    if(track1 > track2) return 1;
    if(track1 == track2) return 0;
    if(track1 < track2) return -1;
    return 0;
}

void LibraryWin_Reload(){
    
    LibraryWin_FreeMemory();

    memset(&selectWindows[0], 0, sizeof(selectWindows[0]));
    memset(&selectWindows[1], 0, sizeof(selectWindows[1]));
    

    FILE *fp = fopen("library", "r");
	if(!fp) return;
    
    int k;

	while(!feof(fp)){

		char path[2048] = {0};
		int pathIndex = 0;

		char c = fgetc(fp);
		while(c != '\n' && !feof(fp)){
			path[pathIndex++] = c;
			c = fgetc(fp);
		}

        int ext = Utils_GetExt(path);

        if(ext == EXTENSION_MP3 || ext == EXTENSION_FLAC){

            SongInfo song = (SongInfo){};

            if(ext == EXTENSION_MP3)
                ID3_Open(path, &song);
            if(ext == EXTENSION_FLAC)
                Flac_GetInfo(path, &song);
			
			song.ext = ext;
			strcpy(song.filepath, path);

            for(k = 0; k < artistIndex; k++){
            	if(strcmp(artists[k].name, song.artist) == 0){
		            artists[k].numOfSongs++;
		            artists[k].songs = (SongInfo *)realloc(artists[k].songs, 
		            	sizeof(SongInfo)*artists[k].numOfSongs);
		            artists[k].songs[artists[k].numOfSongs-1] = song;

		            int j;
		            for(j = 0; j < artists[k].numOfSongs-1; j++){
		            	if(strcmp(artists[k].songs[j].album, song.album) == 0){
		            		if(artists[k].songs[j].image.pixels){
		            			free(artists[k].songs[artists[k].numOfSongs-1].image.pixels);
		            			artists[k].songs[artists[k].numOfSongs-1].image.pixels = NULL;
		            			goto done;
		            		}
		            	}
		            }

		            if(!artists[k].songs[artists[k].numOfSongs-1].image.pixels)
			            artists[k].songs[artists[k].numOfSongs-1].image = LoadDirImage(path);

		            goto done;
            	}
            }
            
            artists = (struct artist *)realloc(artists, sizeof(struct artist)*++artistIndex);
			artists[artistIndex-1].songs = NULL;
            artists[artistIndex-1].numOfSongs = 1;
            artists[artistIndex-1].songs = (SongInfo *)realloc(artists[artistIndex-1].songs, 
            	sizeof(SongInfo)*artists[artistIndex-1].numOfSongs);
            
            artists[artistIndex-1].songs[0] = song;

	        if(!artists[artistIndex-1].songs[0].image.pixels)
	            artists[artistIndex-1].songs[0].image = LoadDirImage(path);
            
            selectWindows[ARTIST_SELECT_WINODW].numOfItems++;
	        strcpy(artists[artistIndex-1].name, song.artist);

			done:
			continue;
        }
	}

	fclose(fp);

	if(artists){

		int k;
		for(k = 0; k < artistIndex; k++){
			if(artists[k].numOfSongs > 0)
			    qsort(artists[k].songs, artists[k].numOfSongs, sizeof(SongInfo), SortSongInfo);
		}

		selectWindows[TRACK_SELECT_WINODW].numOfItems = artists[0].numOfSongs;
	}
	
	hasChanged = 1;
	artistSelectBarHidden = 0;
}

void LibraryWin_Close(){
    LibraryWin_FreeMemory();
}

void LibraryWin_UpLine(){
	selectWindows[inWindow].selectedLine--;
	if(selectWindows[inWindow].selectedLine < 0)
		selectWindows[inWindow].selectedLine = 0;

	if(selectWindows[inWindow].selectedLine-selectWindows[inWindow].scrollOffset < 0)
		selectWindows[inWindow].scrollOffset--;

	if(selectWindows[inWindow].scrollOffset < 0)
		selectWindows[inWindow].scrollOffset = 0;		

	if(inWindow == ARTIST_SELECT_WINODW){
		selectWindows[TRACK_SELECT_WINODW].numOfItems = artists[selectWindows[inWindow].selectedLine].numOfSongs;
		selectWindows[TRACK_SELECT_WINODW].scrollOffset = 0;
		selectWindows[TRACK_SELECT_WINODW].selectedLine = 0;
	}
	hasChanged = 1;
}

void LibraryWin_DownLine(){
	selectWindows[inWindow].selectedLine++;

	if(selectWindows[inWindow].selectedLine >= selectWindows[inWindow].numOfItems-1)
		selectWindows[inWindow].selectedLine = selectWindows[inWindow].numOfItems-1;

	if(selectWindows[inWindow].selectedLine-selectWindows[inWindow].scrollOffset >= height-selectWindowOffsetTop-selectWindowOffsetBottom)
		selectWindows[inWindow].scrollOffset++;

	if(inWindow == ARTIST_SELECT_WINODW){
		selectWindows[TRACK_SELECT_WINODW].numOfItems = artists[selectWindows[inWindow].selectedLine].numOfSongs;
		selectWindows[TRACK_SELECT_WINODW].scrollOffset = 0;
		selectWindows[TRACK_SELECT_WINODW].selectedLine = 0;
	}
	hasChanged = 1;
}

void LibraryWin_SwitchTrackArtist(){
	if(inWindow == ARTIST_SELECT_WINODW) inWindow = TRACK_SELECT_WINODW;
	else if(inWindow == TRACK_SELECT_WINODW) inWindow = ARTIST_SELECT_WINODW;
	hasChanged = 1;
	artistSelectBarHidden = 0;
}

void LibraryWin_ToggleSideBar(){
	artistSelectBarHidden = !artistSelectBarHidden;
	hasChanged = 1;
	inWindow = TRACK_SELECT_WINODW;
}

SongInfo *LibraryWin_GetPlayingSong(){ 
	if(!artists) return NULL;
	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(!artists[artistSelection].songs) return NULL;
	int trackSelection = selectWindows[TRACK_SELECT_WINODW].selectedLine;

	return &artists[artistSelection].songs[trackSelection];
}


static void onSongEnd(){
	hasChanged = 1;

	if(!artists) return;
	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(!artists[artistSelection].songs) return;

    Player_ScrobblePlayingSong();

    SongInfo nextSong;
    if(playingSongIndex+1 < artists[artistSelection].numOfSongs){
        nextSong = artists[artistSelection].songs[++playingSongIndex];
	    selectWindows[TRACK_SELECT_WINODW].selectedLine++;
    } else {
        nextSong = artists[artistSelection].songs[0];
        playingSongIndex = 0;
	    selectWindows[TRACK_SELECT_WINODW].selectedLine = 0;
    }

    if(nextSong.ext == EXTENSION_MP3)
        MP3_Play(nextSong.filepath);
    if(nextSong.ext == EXTENSION_FLAC)
        Flac_Play(nextSong.filepath);
    
    Player_SetPlayingSongStr(nextSong.title);
    Player_SetPlayingSong(nextSong);
    Player_SetEndSongCallback(&onSongEnd);
	hasChanged = 1;
}

void LibraryWin_Select(){ 

	if(!artists) return;

	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(!artists[artistSelection].songs) return;
	int trackSelection = selectWindows[TRACK_SELECT_WINODW].selectedLine;

    if(strcmp(Player_GetPlayingSongStr(), artists[artistSelection].songs[trackSelection].title) == 0){
        Player_ScrobblePlayingSong();
        Player_SetPlayingSong(artists[artistSelection].songs[trackSelection]);
        Pulse_Seek(0);
    } else {
        Player_ScrobblePlayingSong();

        playingSongIndex = trackSelection;

        if(artists[artistSelection].songs[playingSongIndex].ext == EXTENSION_MP3) 
            MP3_Play(artists[artistSelection].songs[playingSongIndex].filepath);

        if(artists[artistSelection].songs[playingSongIndex].ext == EXTENSION_FLAC) 
            Flac_Play(artists[artistSelection].songs[playingSongIndex].filepath);

        Player_SetPlayingSong(artists[artistSelection].songs[playingSongIndex]);
        Player_SetPlayingSongStr(artists[artistSelection].songs[playingSongIndex].title);
	    Player_SetEndSongCallback(&onSongEnd);
    }

    hasChanged = 1;
}

void LibraryWin_SetXYWidthHeight(int newX, int newY, int newWidth, int newHeight){
	x = newX;
	y = newY;
	width = newWidth;
	height = newHeight;
	hasChanged = 1;
}

static int DrawArtistSelectWin(WINDOW *win){

	int maxArtistLen = 0;

	int k;
	for(k = selectWindows[ARTIST_SELECT_WINODW].scrollOffset; k < artistIndex; k++){
		if(strlen(artists[k].name) > maxArtistLen) 
			maxArtistLen = strlen(artists[k].name);
	}

	for(k = selectWindows[ARTIST_SELECT_WINODW].scrollOffset; k < artistIndex; k++){

		int line = k - selectWindows[ARTIST_SELECT_WINODW].scrollOffset;
		
		if(k == selectWindows[ARTIST_SELECT_WINODW].selectedLine && inWindow == ARTIST_SELECT_WINODW)
			wattron(win, COLOR_PAIR(SELECTED_LINE_COLOR_PAIR));
		else if(k == selectWindows[ARTIST_SELECT_WINODW].selectedLine)
			wattron(win, COLOR_PAIR(REVERSE_DEFAULT_COLOR_PAIR));
		else 
			wattron(win, COLOR_PAIR(COLOR_PAIR_DEFAULT));

		int j;
		for(j = 0; j < maxArtistLen+2; j++)
			mvwaddch(win, y+line+selectWindowOffsetTop, x+j, ' ');

		mvwprintw(win, y+line+selectWindowOffsetTop, x+1, "%s", artists[k].name);
	}

	wattron(win, COLOR_PAIR(SEPERATOR_COLOR_PAIR));
	for(k = 0; k < height; k++){
		mvwaddch(win, y+k+titleBarHeight, x+maxArtistLen+2, ACS_VLINE);
	}

	return maxArtistLen+3;
}

static void DrawTrackSelectWin(WINDOW *win, int offsetX, int width){

	if(!artists) return;
	int artistsSelectIndex = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(!artists[artistsSelectIndex].songs) return;

	int k, j;
	
	int maxArtistAlbumLen = 0;
	int maxArtistLen = 0;
	int maxAlbumLen = 0;
	int maxTitleLen = 0;
    
	for(k = selectWindows[TRACK_SELECT_WINODW].scrollOffset; k < artists[artistsSelectIndex].numOfSongs; k++){

		SongInfo song = artists[artistsSelectIndex].songs[k];
        if(strlen(song.artist)+strlen(song.album) > maxArtistAlbumLen) 
            maxArtistAlbumLen = strlen(song.artist)+strlen(song.album);

        if(strlen(song.artist) > maxArtistLen) 
            maxArtistLen = strlen(song.artist);

        if(strlen(song.album) > maxAlbumLen) 
            maxAlbumLen = strlen(song.album);

        if(strlen(song.title) > maxTitleLen) 
            maxTitleLen = strlen(song.title);
    }

	for(k = selectWindows[TRACK_SELECT_WINODW].scrollOffset; k < artists[artistsSelectIndex].numOfSongs; k++){

		SongInfo song = artists[artistsSelectIndex].songs[k];
		
		if(k == selectWindows[TRACK_SELECT_WINODW].selectedLine && inWindow == TRACK_SELECT_WINODW)
			wattron(win, COLOR_PAIR(SELECTED_LINE_COLOR_PAIR));
		else if(k == selectWindows[TRACK_SELECT_WINODW].selectedLine)
			wattron(win, COLOR_PAIR(REVERSE_DEFAULT_COLOR_PAIR));
		else 
			wattron(win, COLOR_PAIR(COLOR_PAIR_DEFAULT));

		int line = k-selectWindows[TRACK_SELECT_WINODW].scrollOffset;

		for(j = 0; j < width; j++)
			mvwaddch(win, y+line+selectWindowOffsetTop, x+j+offsetX, ' ');


    	mvwprintw(win, line+y+selectWindowOffsetTop, x+offsetX+1, song.title);

        if(width-(maxAlbumLen+maxArtistLen+maxTitleLen) > width/3){
        	mvwprintw(win, line+y+selectWindowOffsetTop, x+offsetX+width/2-(maxArtistLen/2), song.artist);
            mvwprintw(win, line+y+selectWindowOffsetTop, x+offsetX+(width-maxAlbumLen)-1, song.album);
        } else {
            mvwprintw(win, line+y+selectWindowOffsetTop, x+offsetX+(width-(maxArtistAlbumLen+3))-1, "%s | %s", song.artist, song.album);
        }
	}
}

void LibraryWin_HasChanged(int has){
	hasChanged = has;
}

static void DrawVisualizer(WINDOW *win, int x, int width){
	int finalLine = selectWindows[TRACK_SELECT_WINODW].numOfItems-selectWindows[TRACK_SELECT_WINODW].scrollOffset;
	if(finalLine < LINES-10){
		VisualizerWin_DrawVisualizerVert(win, 0, x, finalLine+1, width, LINES-finalLine);
	}
}

static int GetTrackAlbumCover(int artistSelection, int trackSelection){
	if(!artists || !artists[artistSelection].songs) return -1;

    int k;
    for(k = 0; k < artists[artistSelection].numOfSongs; k++){
	    if(strcmp(artists[artistSelection].songs[k].album, artists[artistSelection].songs[trackSelection].album) == 0){
	    	if(artists[artistSelection].songs[k].image.pixels) return k;
	    }

    }
    return -1;
}

static void DrawCover(WINDOW *win, int x, int y, int width, int height, int trackSelection){

	if(!artists) return;
	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(artistSelection < artistIndex && !artists[artistSelection].songs) return;

    if(artists[artistSelection].songs[trackSelection].image.pixels){
        
        X11_DrawSongImage(&artists[artistSelection].songs[trackSelection].image, x+1, y+1, width-1, height-1);
    
    } else {

        return;
    }

    wattron(win, COLOR_PAIR(IMAGE_BORDER_COLOR_PAIR));

    int k;
    for(k = x; k < x+width; k++){
        mvwaddch(win, y, k, ACS_HLINE);
        mvwaddch(win, y+height, k, ACS_HLINE);
    }

    for(k = y; k < y+height; k++){
        mvwaddch(win, k, x, ACS_VLINE);
        mvwaddch(win, k, x+width, ACS_VLINE);
    }

    mvwaddch(win, y, x, ACS_BSSB);
    mvwaddch(win, y, x+width, ACS_BBSS); 
    mvwaddch(win, y+height, x+width, ACS_SBBS);
    mvwaddch(win, y+height, x, ACS_SSBB);
}

static void RemoveSongFromLibrary(char *libraryPath, char *filepath){
    
    FILE *fp = fopen(libraryPath, "rb");
    if(!fp) return;

	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	rewind(fp);
	char data[len];
	fread(data, sizeof(char), len, fp);
	rewind(fp);

	int k = 0, foundPos = 0;
    
    while(!feof(fp)){

        int tIndex = 0;
        char temp[4096] = {0};
        char c = fgetc(fp);

        while(c != '\n' && !feof(fp)) {
            temp[tIndex++] = c;
            c = fgetc(fp);
        }
        
        if(strcmp(temp, filepath)==0){
        	foundPos = k;
            break;
        }

        k += tIndex;
    }
	
	fclose(fp);

	if(foundPos != -1){
	    fp = fopen(libraryPath, "w");
	    if(!fp) return;
		memmove(&data[foundPos], &data[foundPos+strlen(filepath)]+1, len-(foundPos+strlen(filepath)-1));
		fwrite(data, sizeof(char), len-strlen(filepath)-1, fp);
		fclose(fp);
	}
}

void LibraryWin_RemoveSelectedFromLibrary(){
	if(!artists) return;
	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	if(artistSelection < artistIndex && !artists[artistSelection].songs) return;
	int trackSelection = selectWindows[TRACK_SELECT_WINODW].selectedLine;

    RemoveSongFromLibrary("library", artists[artistSelection].songs[trackSelection].filepath);
}

void LibraryWin_UpdateDraw(WINDOW *win){

	if(!hasChanged) {
		DrawVisualizer(win, maxArtistLen, newWidth - maxArtistLen);
		return;
	}
    
	int artistSelection = selectWindows[ARTIST_SELECT_WINODW].selectedLine;
	int trackSelection = selectWindows[TRACK_SELECT_WINODW].selectedLine;
	int albumCoverTrack;

    newWidth = width;

    if((albumCoverTrack = GetTrackAlbumCover(artistSelection, trackSelection)) != -1)
		newWidth = width/2;
	else
	    X11_WithdrawWindow();

	int k, j;
	for(k = 0; k < height; k++){
		for(j = 0; j < width; j++){
			mvwaddch(win, k, j, ' ');
		}
	}

    wmove(win, 0,0);
    wbkgdset(win, COLOR_PAIR(TITLE_COLOR_PAIR));
    Utils_FormatStr(win, newWidth, "%c%s%cVolume: %i%%",  FORMAT_STR_SEPERATOR, Player_GetPlayingSongStr(), FORMAT_STR_SEPERATOR, Pulse_GetVolume());
	wattron(win, COLOR_PAIR(COLOR_PAIR_DEFAULT));

	wbkgdset(win, COLOR_PAIR(COLOR_PAIR_DEFAULT));
	maxArtistLen = 0;
	if(!artistSelectBarHidden) maxArtistLen = DrawArtistSelectWin(win);
	DrawTrackSelectWin(win, maxArtistLen, newWidth - maxArtistLen);
	DrawVisualizer(win, maxArtistLen, newWidth - (maxArtistLen));

	if(albumCoverTrack != -1)
		DrawCover(win, newWidth+1, 0, newWidth-2, height-1- titleBarHeight, albumCoverTrack);
	
	hasChanged = 0;
}