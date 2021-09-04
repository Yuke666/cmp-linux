#ifndef LIBRARY_WIN_DEF
#define LIBRARY_WIN_DEF

#include "ui.h"

void LibraryWin_UpdateDraw(WINDOW *win);
void LibraryWin_UpLine();
void LibraryWin_DrawCover(WINDOW *win);
void LibraryWin_DownLine();
void LibraryWin_UpLine();
void LibraryWin_Select();
void LibraryWin_Resize();
void LibraryWin_Reload();
void LibraryWin_SetXYWidthHeight(int newX, int newY, int newWidth, int newHeight);
void LibraryWin_SwitchTrackArtist();
void LibraryWin_Close();
void LibraryWin_ToggleSideBar();
void LibraryWin_HasChanged(int has);
void LibraryWin_RemoveSelectedFromLibrary();

#endif