#include <lua.h>
#include <string.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"
#include "x11.h"
#include "lastfm.h"
#include "browser_win.h"

static lua_State *L;

char browserCmd[1024] = "firefox";

static int SetColorPair(lua_State *L){
	int which = lua_tonumber(L,1);
	int color1 = lua_tonumber(L,2);
	int color2 = lua_tonumber(L,3);
    init_pair(which, color1, color2);
    return 0;
}

static int SetHotkey(lua_State *L){
	int which = lua_tonumber(L,1);
	int key = lua_tonumber(L,2);
    UI_SetHotkey(which, key);
    return 0;
}

static int SetGlobalHotkey(lua_State *L){
	int which = lua_tonumber(L,1);
	int key = lua_tonumber(L,2);
    UI_SetGlobalHotkey(which, key);
    return 0;
}

static int SetGlobalKey(lua_State *L){
	int key = lua_tonumber(L,1);
    UI_SetGlobalKey( key);
    return 0;
}

static int SetStartDirectory(lua_State *L){
	const char *path = lua_tostring(L, 1);
	BrowserWin_SetInDirectory(path);
	return 0;
}

static int UseLastFM(lua_State *L){
	if(lua_tonumber(L,1)){
	    LastFM_Init();
	    clear();
	    mvwprintw(stdscr, LINES/2, COLS/2-(strlen("Logging into LastFM..")/2), "Logging into LastFM..");
	    refresh();
		LastFM_Authenticate((char*)lua_tostring(L,2), (char*)lua_tostring(L,3));
	}
	return 0;
}

static int SetBrowserCmd(lua_State *L){
	strcpy(browserCmd, lua_tostring(L,1));
	return 0;
}

char *Config_GetBrowserCmd(){
	return browserCmd;
}

void Config_Load(const char *path){
	L = luaL_newstate();
	luaopen_base(L);
	luaopen_string(L);
	luaopen_io(L);
	luaopen_table(L);
	luaopen_math(L);
	luaopen_io(L);
	luaopen_debug(L);
	lua_register(L, "SetColorPair", SetColorPair);
	lua_register(L, "SetHotkey", SetHotkey);
	lua_register(L, "SetGlobalHotkey", SetGlobalHotkey);
	lua_register(L, "SetGlobalKey", SetGlobalKey);
	lua_register(L, "SetStartDirectory", SetStartDirectory);
	lua_register(L, "UseLastFM", UseLastFM);
	lua_register(L, "SetBrowserCmd", SetBrowserCmd);
	lua_pushnumber(L, TITLE_COLOR_PAIR);
	lua_setglobal(L, "TITLE_COLOR_PAIR");
	lua_pushnumber(L, KEY_ENTER);
	lua_setglobal(L, "KEY_ENTER");
	lua_pushnumber(L, ADD_TO_LIBRARY_KEY);
	lua_setglobal(L, "ADD_TO_LIBRARY_KEY");
	lua_pushnumber(L, FILE_LINE_COLOR_PAIR);
	lua_setglobal(L, "FILE_LINE_COLOR_PAIR");
	lua_pushnumber(L, DIRECTORY_LINE_COLOR_PAIR);
	lua_setglobal(L, "DIRECTORY_LINE_COLOR_PAIR");
	lua_pushnumber(L, SELECTED_LINE_COLOR_PAIR);
	lua_setglobal(L, "SELECTED_LINE_COLOR_PAIR");
	lua_pushnumber(L, BROWSER_COLOR_PAIR);
	lua_setglobal(L, "BROWSER_COLOR_PAIR");
	lua_pushnumber(L, VISUALIZER_BACKGROUND_COLOR_PAIR);
	lua_setglobal(L, "VISUALIZER_BACKGROUND_COLOR_PAIR");
	lua_pushnumber(L, IMAGE_BORDER_COLOR_PAIR);
	lua_setglobal(L, "IMAGE_BORDER_COLOR_PAIR");
	lua_pushnumber(L, COLOR_BLACK);
	lua_setglobal(L, "COLOR_BLACK");
	lua_pushnumber(L, COLOR_RED);
	lua_setglobal(L, "COLOR_RED");
	lua_pushnumber(L, COLOR_GREEN);
	lua_setglobal(L, "COLOR_GREEN");
	lua_pushnumber(L, COLOR_YELLOW);
	lua_setglobal(L, "COLOR_YELLOW");
	lua_pushnumber(L, COLOR_BLUE);
	lua_setglobal(L, "COLOR_BLUE");
	lua_pushnumber(L, COLOR_MAGENTA);
	lua_setglobal(L, "COLOR_MAGENTA");
	lua_pushnumber(L, COLOR_CYAN);
	lua_setglobal(L, "COLOR_CYAN");
	lua_pushnumber(L, COLOR_WHITE);
	lua_setglobal(L, "COLOR_WHITE");
	lua_pushnumber(L, VOLUME_UP_KEY);
	lua_setglobal(L, "VOLUME_UP_KEY");
	lua_pushnumber(L, VOLUME_DOWN_KEY);
	lua_setglobal(L, "VOLUME_DOWN_KEY");
	lua_pushnumber(L, PAUSE_KEY);
	lua_setglobal(L, "PAUSE_KEY");
	lua_pushnumber(L, BROWSER_WIN_KEY);
	lua_setglobal(L, "BROWSER_WIN_KEY");
	lua_pushnumber(L, VISUALIZER_WIN_KEY);
	lua_setglobal(L, "VISUALIZER_WIN_KEY");
	lua_pushnumber(L, QUIT_KEY);
	lua_setglobal(L, "QUIT_KEY");
	lua_pushnumber(L, SEEK_LEFT_KEY);
	lua_setglobal(L, "SEEK_LEFT_KEY");
	lua_pushnumber(L, SEEK_RIGHT_KEY);
	lua_setglobal(L, "SEEK_RIGHT_KEY");
	lua_pushnumber(L, DOWN_LINE_KEY);
	lua_setglobal(L, "DOWN_LINE_KEY");
	lua_pushnumber(L, UP_LINE_KEY);
	lua_setglobal(L, "UP_LINE_KEY");
	lua_pushnumber(L, SELECT_KEY);
	lua_setglobal(L, "SELECT_KEY");
	lua_pushnumber(L, X11_CONTROL_KEY);
	lua_setglobal(L, "GLOBAL_CONTROL_KEY");
	lua_pushnumber(L, X11_ALT_KEY);
	lua_setglobal(L, "GLOBAL_ALT_KEY");
	lua_pushnumber(L, X11_SUPER_KEY);
	lua_setglobal(L, "GLOBAL_SUPER_KEY");
	luaL_dofile(L, path);
	lua_close(L);
}