#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "ui.h"
#include "browser_win.h"
#include "library_win.h"
#include "pulse.h"
#include "config.h"
#include "x11.h"
#include "lastfm.h"
#include "player.h"

int main(int argc, char **argv){

    X11_Init();
    Pulse_Open();
    UI_Open();
 	Config_Load("config.lua");
    BrowserWin_Reload();
	Player_Init();

    while(1){
        if(!UI_UpdateDraw())
        	break;
   }

    LastFM_Close();
    X11_Close();
    UI_Close();
    Pulse_Close();
    LibraryWin_Close();
    BrowserWin_Close();
    return 0;
}