#ifndef BROSWER_WIN_DEF
#define BROSWER_WIN_DEF

#include "ui.h"
#include "songinfo.h"
#include <time.h>

int BrowserWin_UpdateDraw(WINDOW *win);
int BrowserWin_Reload();
void BrowserWin_Close();
void BrowserWin_HasChanged(int has);
void BrowserWin_ResetScroll();
void BrowserWin_SetOffsetLeft(int l);
void BrowserWin_SetOffsetRight(int l);
void BrowserWin_SetOffsetTop(int l);
void BrowserWin_SetOffsetBottom(int l);
void BrowserWin_SetInDirectory(const char *path);
SongInfo BrowserWin_GetSelectedSong();
SongInfo *BrowserWin_GetPlayingSong();
void BrowserWin_DrawCover(WINDOW *win);
void BrowserWin_DownLine();
void BrowserWin_UpLine();
void BrowserWin_Select();
void BrowserWin_Resize();
void BrowserWin_AddSelectedToLibrary();


#endif