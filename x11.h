#ifndef X11_DEF
#define X11_DEF

#include "songinfo.h"
#include "ui.h"

#define X11_CONTROL_KEY KEY_MAX+1
#define X11_ALT_KEY KEY_MAX+2
#define X11_SUPER_KEY KEY_MAX+3
#define X11_NUM_OF_RET_KEYS 32*8

void X11_WithdrawWindow();
void X11_DrawSongImage(SongImage *img, int xPos, int yPos, int drawWidth,int drawHeight);
void X11_Init();
void X11_Close();
int X11_GetGlobalKeys(int *ret);

#endif