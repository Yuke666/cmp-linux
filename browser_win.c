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
#include "player.h"
#include "mp3.h"
#include "flac.h"
#include "ui.h"
#include "id3.h"
#include "x11.h"

static int offsetTop = 1;
static int offsetBottom = 1;
static int offsetLeft = 0;
static int offsetRight = 0;
static int imageOffsetRight = 1;
static int imageOffsetLeft = 1;
static int imageOffsetTop = 0;
static int imageOffsetBottom = 2;
static int selectedLine = 0;
static int hasChanged = 1;
static int scrollOffset = 0;
static int directoriesIndex = 0;
static int songsIndex = 0;
static int playingSongIndex = 0;
static char inDirectory[2048] = "/home";
static SongImage inDirectoryImage;

static struct directory {
    char path[2048];
    SongImage image;
} *directories = NULL;

static struct song {
    char filepath[2048];
    SongInfo info;
    int ext;
} *songs = NULL;


static int QSortFuncStructSong(const void *one, const void *two){
    char *artist1 = ((struct song *)one)->info.artist;
    char *artist2 = ((struct song *)two)->info.artist;
    int res = Utils_QSortStr(artist1, artist2);
    if(res != 0) return res;
    char *album1 = ((struct song *)one)->info.album;
    char *album2 = ((struct song *)two)->info.album;
    res = Utils_QSortStr(album1, album2);
    if(res != 0) return res;
    int track1 = ((struct song *)one)->info.track;
    int track2 = ((struct song *)two)->info.track;
    if(track1 > track2) return 1;
    if(track1 == track2) return 0;
    if(track1 < track2) return -1;
    return 0;
}

static int QSortFuncStructDirectory(const void *one, const void *two){
    char *name1 = ((struct directory *)one)->path;
    char *name2 = ((struct directory *)two)->path;
    return Utils_QSortStr(name1, name2);
}

void BrowserWin_Close(){
    int k;
    for(k = 0; k < songsIndex; k++) 
        if(songs[k].info.image.pixels) 
            free(songs[k].info.image.pixels);

    if(songs) free(songs);
    if(directories) free(directories);

    songs = NULL;
    directories = NULL;
    if(inDirectoryImage.pixels) free(inDirectoryImage.pixels);
    inDirectoryImage.pixels = NULL;
    directoriesIndex = 0;
    songsIndex = 0;
}

int BrowserWin_Reload(){

    BrowserWin_Close();

    DIR *dir;
    struct dirent *dp;
    dir = opendir(inDirectory);
    if(dir == NULL) return 0;
    
    while((dp = readdir(dir)) != NULL){

        struct stat s;
        char temp[4096];
    	strcpy(temp, inDirectory);
        
        if(strcmp(dp->d_name, "..") == 0){
        	int f;
        	for(f = strlen(temp); f > 0; f--){
        		if(temp[f] == '/') {
        			temp[f] = 0;
        			break;
        		}
        	}
        } else {
        	sprintf(temp, "%s/%s", temp, dp->d_name);
        }

        stat(temp, &s);

        if((s.st_mode & S_IFMT) == S_IFDIR){

        	if(strcmp(dp->d_name, ".") == 0) continue;

        	directoriesIndex++;
        	directories = (struct directory *) realloc(directories, directoriesIndex*sizeof(struct directory));

            sprintf(directories[directoriesIndex-1].path,"%s/", temp);
        
        } else {
            int ext = Utils_GetExt(dp->d_name);
            if(ext == EXTENSION_MP3 || ext == EXTENSION_FLAC){

	        	songsIndex++;
	        	songs = (struct song *) realloc(songs, songsIndex*sizeof(struct song));

                songs[songsIndex-1].ext = ext;
                
                if(ext == EXTENSION_MP3)
                    ID3_Open(temp, &songs[songsIndex-1].info);
                if(ext == EXTENSION_FLAC)
                    Flac_GetInfo(temp, &songs[songsIndex-1].info);

		        strcpy(songs[songsIndex-1].filepath, temp);
            }
            if(ext == EXTENSION_JPEG || ext == EXTENSION_PNG){
                FILE *tempFile = fopen(temp, "rb");
                
                if(inDirectoryImage.pixels) free(inDirectoryImage.pixels);
                inDirectoryImage.pixels = NULL;

                if(ext == EXTENSION_JPEG)
                    if(!Utils_LoadJPEG(tempFile, &inDirectoryImage))
                        Utils_LoadPNG(tempFile, &inDirectoryImage);

                if(ext == EXTENSION_PNG)
                    if(!Utils_LoadPNG(tempFile, &inDirectoryImage))
                        Utils_LoadJPEG(tempFile, &inDirectoryImage);


                fclose(tempFile);
            }
        }
    }

    closedir(dir);
    qsort(directories, directoriesIndex, sizeof(struct directory), QSortFuncStructDirectory);
    qsort(songs, songsIndex, sizeof(struct song), QSortFuncStructSong);

    hasChanged =1;
    
    return 1;
}

static void ClearRegion(WINDOW *win){
	int k, m;
	for(k = 0; k < LINES-offsetBottom; k++){
		for(m = offsetLeft; m < (COLS-offsetRight)+imageOffsetLeft; m++){
			mvwprintw(win, k,m," ");
		}
	}
}

static void DrawVisualizer(WINDOW *win){
	int finalLine = ((songsIndex+directoriesIndex)-scrollOffset)+offsetTop;
	if(finalLine < LINES-10){
		VisualizerWin_DrawVisualizerVert(win, 0, offsetLeft, finalLine, COLS-offsetRight, LINES-finalLine-offsetBottom);
	}
}

void BrowserWin_DrawCover(WINDOW *win){

    int line = (scrollOffset-directoriesIndex)+selectedLine > 0 ? (scrollOffset-directoriesIndex)+selectedLine : 0; 

    if(songs && line < songsIndex && songs[line].info.image.pixels){
        
        X11_DrawSongImage(&songs[line].info.image, COLS-offsetRight+imageOffsetLeft+1, 
            imageOffsetTop+1, offsetRight-imageOffsetRight-imageOffsetLeft-1, LINES-offsetBottom-imageOffsetBottom);
    
    } else if(inDirectoryImage.pixels) {

        X11_DrawSongImage(&inDirectoryImage, COLS-offsetRight+imageOffsetLeft+1, 
            imageOffsetTop+1, offsetRight-imageOffsetRight-imageOffsetLeft-1, LINES-offsetBottom-imageOffsetBottom);
    
    } else {

        return;
    }

    wattron(win, COLOR_PAIR(IMAGE_BORDER_COLOR_PAIR));

    int k;
    for(k = COLS-offsetRight+imageOffsetLeft; k < COLS; k++){
        mvwaddch(win, imageOffsetTop, k, ACS_HLINE);
        mvwaddch(win, LINES-imageOffsetBottom, k, ACS_HLINE);
    }

    for(k = imageOffsetTop; k < LINES-imageOffsetBottom; k++){
        mvwaddch(win, k, COLS-offsetRight+imageOffsetLeft, ACS_VLINE);
        mvwaddch(win, k, COLS-imageOffsetRight, ACS_VLINE);
    }

    mvwaddch(win, imageOffsetTop, COLS-offsetRight+imageOffsetLeft, ACS_BSSB);
    mvwaddch(win, imageOffsetTop, COLS-imageOffsetRight, ACS_BBSS);
    mvwaddch(win, LINES-imageOffsetBottom, COLS-imageOffsetRight, ACS_SBBS);
    mvwaddch(win, LINES-imageOffsetBottom, COLS-offsetRight+imageOffsetLeft, ACS_SSBB);
}

int BrowserWin_UpdateDraw(WINDOW *win){

	if(!hasChanged || (!songs && !directories)) {
		DrawVisualizer(win);
		return 0;
	}

	ClearRegion(win);

	int lastOffsetRight = offsetRight;
    int line = (scrollOffset-directoriesIndex)+selectedLine > 0 ? (scrollOffset-directoriesIndex)+selectedLine : 0; 

    if((songs && line < songsIndex && songs[line].info.image.pixels) || inDirectoryImage.pixels)
        offsetRight = (COLS/2)+1;
    else {
        X11_WithdrawWindow();
        offsetRight = 0;
    }

    if(lastOffsetRight != offsetRight) ClearRegion(win);
	
    line = scrollOffset-directoriesIndex > 0 ? scrollOffset-directoriesIndex : 0; 

    wbkgdset(win, COLOR_PAIR(TITLE_COLOR_PAIR));

    wmove(win, 0,offsetLeft);
    
    Utils_FormatStr(win, COLS-offsetRight-offsetLeft, "%c%s%cVolume: %i%%",  
    	FORMAT_STR_SEPERATOR, Player_GetPlayingSongStr(), FORMAT_STR_SEPERATOR, Pulse_GetVolume());

	wbkgdset(win, COLOR_PAIR(SELECTED_LINE_COLOR_PAIR));

	int m, k;
    for(m = scrollOffset; m < directoriesIndex; m++){

    	int line = m-scrollOffset;
    	if(line+offsetTop >= LINES-offsetBottom) break;

    	if(line == selectedLine) wbkgdset(win, COLOR_PAIR(SELECTED_LINE_COLOR_PAIR));
	    else wbkgdset(win, COLOR_PAIR(FILE_LINE_COLOR_PAIR));
 	
 		int f;
 		for(f = offsetLeft; f < COLS-offsetRight; f++)
	    	mvwprintw(win, line+offsetTop, f, " ");

    	mvwprintw(win, line+offsetTop, offsetLeft+1, directories[m].path); 
    }
    	
    int maxAlbumArtistLen = 0;
    int maxArtistLen = 0;
    int maxAlbumLen = 0;

    for(k = 0; k < songsIndex; k++){
        if(strlen(songs[k].info.artist)+strlen(songs[k].info.album) > maxAlbumArtistLen) 
            maxAlbumArtistLen = strlen(songs[k].info.artist)+strlen(songs[k].info.album);

        if(strlen(songs[k].info.artist) > maxArtistLen) 
            maxArtistLen = strlen(songs[k].info.artist);

        if(strlen(songs[k].info.album) > maxAlbumLen) 
            maxAlbumLen = strlen(songs[k].info.album);
    }

    for(k = line; k < songsIndex; k++){
    	
    	SongInfo song = songs[k].info;

    	int line = k+(m-scrollOffset);
    	if(scrollOffset-directoriesIndex > 0)
    		line = (k-(scrollOffset-directoriesIndex))+(m-scrollOffset);

    	if(line+offsetTop >= LINES-offsetBottom) break;

    	if(line == selectedLine) wbkgdset(win, COLOR_PAIR(SELECTED_LINE_COLOR_PAIR)); 
	    else wbkgdset(win, COLOR_PAIR(FILE_LINE_COLOR_PAIR));
 		
 		int f;
 		for(f = offsetLeft; f < COLS-offsetRight; f++)
	    	mvwprintw(win, line+offsetTop, f, " ");
    	
    	mvwprintw(win, line+offsetTop, offsetLeft+1, song.title);

        if(offsetRight == 0){
        	mvwprintw(win, line+offsetTop, offsetLeft+((COLS-offsetRight)/2) - (maxArtistLen/2), song.artist);
            mvwprintw(win, line+offsetTop, offsetLeft+COLS-maxAlbumLen-1 - offsetRight, song.album);
        } else {
            mvwprintw(win, line+offsetTop, offsetLeft+(COLS-maxAlbumArtistLen-3- offsetRight), "%s | %s", song.artist, song.album);
        }
    }

    wbkgdset(win, COLOR_PAIR(COLOR_PAIR_DEFAULT));
	DrawVisualizer(win);
    BrowserWin_DrawCover(win);
    
    hasChanged = 0; 

    return 1;
}

void BrowserWin_SetImageOffsetLeft(int l){ imageOffsetLeft = l; }
void BrowserWin_SetImageOffsetRight(int l){ imageOffsetRight = l; }
void BrowserWin_SetImageOffsetTop(int l){ imageOffsetTop = l; }
void BrowserWin_SetImageOffsetBottom(int l){ imageOffsetBottom = l; }
void BrowserWin_SetOffsetLeft(int l){ offsetLeft = l; }
void BrowserWin_SetOffsetRight(int l){ offsetRight = l; }
void BrowserWin_SetOffsetTop(int l){ offsetTop = l; }
void BrowserWin_SetOffsetBottom(int l){ offsetBottom = l; }
void BrowserWin_HasChanged(int has) { hasChanged = has; }
void BrowserWin_SetInDirectory(const char *path){ strcpy(inDirectory, path); }

SongInfo *BrowserWin_GetPlayingSong(){ 
    if(!songs || playingSongIndex > songsIndex) return NULL;
    return &songs[playingSongIndex].info; 
}

static void onSongEnd(){

    Player_ScrobblePlayingSong();
    
    if(songs){
        struct song nextSong;

        if(playingSongIndex+1 < songsIndex)
            nextSong = songs[++playingSongIndex];
        else
            nextSong = songs[playingSongIndex = 0];

        if(nextSong.ext == EXTENSION_MP3)
            MP3_Play(nextSong.filepath);
        if(nextSong.ext == EXTENSION_FLAC)
            Flac_Play(nextSong.filepath);
        
        Player_SetPlayingSongStr(nextSong.info.title);
        selectedLine = playingSongIndex+directoriesIndex;
        Player_SetPlayingSong(nextSong.info);
    }
    Player_SetEndSongCallback(&onSongEnd);
	hasChanged = 1;
}

void BrowserWin_DownLine(){
    hasChanged = 1;

    selectedLine++;

    if(selectedLine+offsetTop >= LINES-offsetBottom){
        
        if(scrollOffset+1+selectedLine < directoriesIndex+songsIndex)
            scrollOffset++;

        selectedLine--;
    }

    if(selectedLine > directoriesIndex+songsIndex-1) 
            selectedLine = directoriesIndex+songsIndex-1;

}

void BrowserWin_UpLine(){
    hasChanged = 1;
    selectedLine--;

    if(selectedLine < 0 && scrollOffset > 0) {
        
        scrollOffset--;
        if(scrollOffset < 0) scrollOffset = 0;
        selectedLine = 0;
    }

    if(selectedLine < 0) 
        selectedLine = 0;
}

static void AddSongToLibrary(FILE *fp, int newFile, char *filepath){
    if(!newFile){
        fseek(fp, 0, 0);
        while(!feof(fp)){

            int tIndex = 0;
            char temp[4096] = {0};
            char c = fgetc(fp);

            while(c != '\n' && !feof(fp)) {
                temp[tIndex++] = c;
                c = fgetc(fp);
            }
            
            if(strcmp(temp, filepath)==0)
                return;
        }

        fseek(fp, 0, SEEK_END);
    }
    fwrite(filepath, sizeof(char), strlen(filepath), fp);
    fwrite("\n", sizeof(char), strlen("\n"), fp);
}

void BrowserWin_AddSelectedToLibrary(){
    
    BrowserWin_DownLine();

    int newFile = 0;
    FILE *fp = fopen("library", "rw+");
    if(!fp) { fp = fopen("library", "w"); newFile = 1; }
    if(!fp) return;

    if(selectedLine < directoriesIndex){

        DIR *dir;
        struct dirent *dp;
        dir = opendir(directories[selectedLine].path);
        if(dir == NULL) return;
        
        while((dp = readdir(dir)) != NULL){

            struct stat s;
            char temp[4096];
            strcpy(temp, directories[selectedLine].path);
            sprintf(temp, "%s/%s", temp, dp->d_name);
            stat(temp, &s);

            if((s.st_mode & S_IFMT) != S_IFDIR){
                int ext = Utils_GetExt(dp->d_name);
                if(ext == EXTENSION_MP3 || ext == EXTENSION_FLAC){
                    AddSongToLibrary(fp, newFile, temp);
                }
            }
        }

        closedir(dir);
        fclose(fp);
        return;
    }
    
    if(!songs || selectedLine-directoriesIndex > songsIndex){
        fclose(fp);
        return;
    }

    int line = selectedLine-directoriesIndex;
    AddSongToLibrary(fp, newFile, songs[line].filepath);
    fclose(fp);
}

void BrowserWin_Select(){
    hasChanged = 1;
    if(selectedLine < directoriesIndex){
        memset(inDirectory, '\0', sizeof(inDirectory));
        strcpy(&inDirectory[0], directories[selectedLine].path);
        inDirectory[strlen(inDirectory)-1] = 0;
        selectedLine = 0;
        scrollOffset = 0;

        if(inDirectoryImage.pixels) free(inDirectoryImage.pixels);
        inDirectoryImage.pixels = NULL;

        BrowserWin_Reload();
    }

    if(selectedLine+scrollOffset < directoriesIndex+songsIndex && 
        selectedLine+scrollOffset > directoriesIndex-1){

        if(strcmp(Player_GetPlayingSongStr(), songs[selectedLine+scrollOffset-directoriesIndex].info.title) == 0){
            Player_ScrobblePlayingSong();
            Player_SetPlayingSong(songs[playingSongIndex].info);
            Pulse_Seek(0);
        } else {
            Player_ScrobblePlayingSong();

            playingSongIndex = scrollOffset+selectedLine-directoriesIndex;

            if(songs[playingSongIndex].ext == EXTENSION_MP3) 
                MP3_Play(songs[playingSongIndex].filepath);

            if(songs[playingSongIndex].ext == EXTENSION_FLAC) 
                Flac_Play(songs[playingSongIndex].filepath);

            Player_SetPlayingSongStr(songs[playingSongIndex].info.title);
            Player_SetPlayingSong(songs[playingSongIndex].info);
            Player_SetEndSongCallback(&onSongEnd);
        }
    }
}

void BrowserWin_Resize(){
    selectedLine = 0;
    scrollOffset = 0;
    hasChanged = 1;
}