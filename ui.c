#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "x11.h"
#include "browser_win.h"
#include "library_win.h"
#include "lastfm.h"
#include "pulse.h"
#include "visualizer_win.h"

enum {
    BROWSER_WINDOW,
    VISUALIZER_WINDOW,
    LIBRARY_WINDOW,
};


static int ui_keys[REMOVE_FROM_LIBRARY_KEY+1] = {
    '-',
    '=',
    'c',
    '1',
    '2',
    'q',
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    10,
    'a',
    '3',
    '\t',
    ' ',
    'x',
};

static int global_keys[REMOVE_FROM_LIBRARY_KEY+1] = {
    '-',
    '=',
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

static int activeWindow = BROWSER_WINDOW;
static float volume = 100;
static int paused = 0;
static int lastLines, lastCols;
static int globalKey = X11_CONTROL_KEY;

void UI_Open(){
    initscr();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(TITLE_COLOR_PAIR,                 -1,            COLOR_BLUE);
    init_pair(FILE_LINE_COLOR_PAIR,             -1,            -1);
    init_pair(SELECTED_LINE_COLOR_PAIR,         -1,            COLOR_BLUE);
    init_pair(DIRECTORY_LINE_COLOR_PAIR,        COLOR_RED,     -1);
    init_pair(SEPERATOR_COLOR_PAIR,             COLOR_BLUE,    -1);
    init_pair(COLOR_PAIR_DEFAULT,               -1,            -1);
    init_pair(IMAGE_BORDER_COLOR_PAIR,          -1,            -1);
    init_pair(REVERSE_DEFAULT_COLOR_PAIR,       COLOR_BLACK,   COLOR_WHITE);
    init_pair(VISUALIZER_BACKGROUND_COLOR_PAIR, -1 ,           -1);
    init_pair(CYAN_BLACK_COLOR_PAIR,            COLOR_CYAN,    -1);
    init_pair(BLUE_BLACK_COLOR_PAIR,            COLOR_BLUE,    -1);
    init_pair(MAGENTA_BLACK_COLOR_PAIR,         COLOR_MAGENTA, -1);
    init_pair(GREEN_BLACK_COLOR_PAIR,           COLOR_GREEN,   -1);
    init_pair(YELLOW_BLACK_COLOR_PAIR,          COLOR_YELLOW,  -1);
    init_pair(RED_BLACK_COLOR_PAIR,             COLOR_RED,     -1);
    activeWindow = BROWSER_WINDOW;
    volume = Pulse_GetVolume();
    wclear(stdscr);
    timeout(50);
}

void UI_SetHotkey(int type, int key){
    ui_keys[type] = key;
}

void UI_SetGlobalHotkey(int type, int key){
    global_keys[type] = key;
}

void UI_SetGlobalKey( int key){
    globalKey = key;
}

void UI_Close(){
    wclear(stdscr);
    wrefresh(stdscr);
    endwin();
}

static int handleKeyDown(int key, int *keys){

    if(key == -1) return 1;

    if(key == keys[BROWSER_WIN_KEY]) {
        activeWindow = BROWSER_WINDOW;
        wclear(stdscr);
        BrowserWin_HasChanged(1);
        BrowserWin_Reload();
        LibraryWin_Close();
    }
    
    if(key == keys[VISUALIZER_WIN_KEY]) { 
        activeWindow = VISUALIZER_WINDOW; 
        wclear(stdscr); 
        X11_WithdrawWindow();
        BrowserWin_Close();
        LibraryWin_Close();
    }
    
    if(key == keys[LIBRARY_WIN_KEY]) { 
        activeWindow = LIBRARY_WINDOW; 
        wclear(stdscr); 
        BrowserWin_Close();
        LibraryWin_Reload();
    }

    if(key == keys[QUIT_KEY]) return 0;
    
    if(key == keys[PAUSE_KEY]) { 
    
        if(paused) 
            Pulse_Unpause(NULL); 
        else 
            Pulse_Pause(); 
    
        paused = !paused;
    }

    if(key == KEY_RESIZE || lastCols != COLS || lastLines != LINES){
        lastLines = LINES;
        lastCols = COLS;
        X11_WithdrawWindow();
        if(activeWindow == BROWSER_WINDOW){
            BrowserWin_Resize();
            BrowserWin_UpdateDraw(stdscr);
        }
        LibraryWin_SetXYWidthHeight(0, 0, COLS, LINES);
    }

    if(key == keys[SEEK_LEFT_KEY]) Pulse_Seek(Pulse_GetSeek()-0.1);
    if(key == keys[SEEK_RIGHT_KEY]) Pulse_Seek(Pulse_GetSeek()+0.1);
    
    if(activeWindow == BROWSER_WINDOW){ 
        if(key == keys[UP_LINE_KEY]) BrowserWin_UpLine();
        if(key == keys[DOWN_LINE_KEY]) BrowserWin_DownLine();
        if(key == keys[SELECT_KEY]) BrowserWin_Select();
        if(key == keys[ADD_TO_LIBRARY_KEY]) BrowserWin_AddSelectedToLibrary();
    }

    if(activeWindow == LIBRARY_WINDOW){
        if(key == keys[UP_LINE_KEY]) LibraryWin_UpLine();
        if(key == keys[DOWN_LINE_KEY]) LibraryWin_DownLine();
        if(key == keys[SELECT_KEY]) LibraryWin_Select();
        if(key == keys[LIBRARY_WIN_TOGGLE_SIDE_KEY]) LibraryWin_ToggleSideBar();
        if(key == keys[LIBRARY_WIN_SWITCH_KEY]) LibraryWin_SwitchTrackArtist();
        if(key == keys[REMOVE_FROM_LIBRARY_KEY]) LibraryWin_RemoveSelectedFromLibrary();
    }

    if(key == keys[VOLUME_DOWN_KEY] || key == keys[VOLUME_UP_KEY]) {
        if(key == keys[VOLUME_DOWN_KEY]) volume -= 10;
        if(key == keys[VOLUME_UP_KEY]) volume += 10;

        if(volume < 0) volume = 0;
        if(volume > 100) volume = 100;

        Pulse_SetVolume(volume);
        if(activeWindow == BROWSER_WINDOW){
            BrowserWin_HasChanged(1);
            BrowserWin_UpdateDraw(stdscr);
        }
        if(activeWindow == LIBRARY_WINDOW){
            LibraryWin_HasChanged(1);
            LibraryWin_UpdateDraw(stdscr);
        }
    }


    return 1;
}

int handleGlobalKeyDown(int *keys){

    int globalDown = 0;
    int k;
    for(k = 0; k < X11_NUM_OF_RET_KEYS; k++){
        if(keys[k] == globalKey) globalDown = 1;
    }

    if(!globalDown) return 1;

    for(k = 0; k < X11_NUM_OF_RET_KEYS; k++){
        if(!keys[k]) continue;
        if(!handleKeyDown(keys[k], global_keys)) return 0;
    }

    return 1;
}

int UI_UpdateDraw(){
    
    int key = getch();

    int ret[X11_NUM_OF_RET_KEYS];
    memset(ret, 0, sizeof(ret));

    if(X11_GetGlobalKeys(ret))
        if(!handleGlobalKeyDown(ret)) return 0;

    if(!handleKeyDown(key, ui_keys)) return 0;

    if(activeWindow == BROWSER_WINDOW) BrowserWin_UpdateDraw(stdscr);
    else if(activeWindow == VISUALIZER_WINDOW) VisualizerWin_UpdateDraw(stdscr);
    else if(activeWindow == LIBRARY_WINDOW) LibraryWin_UpdateDraw(stdscr);

    int seek = Pulse_GetSeek() * COLS;
    attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));

    int k;
    for(k = 0; k < seek-1; k++) mvwprintw(stdscr, LINES-1, k, "=");
    for(       ; k < COLS; k++) mvwprintw(stdscr, LINES-1, k, " ");
    
    mvwprintw(stdscr, LINES-1, seek-1, ">");

    wrefresh(stdscr);


    return 1;
}