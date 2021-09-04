#include "visualizer_win.h"
#include "pulse.h"
#include "ui.h"
#include <stdio.h>
#include <math.h>

void VisualizerWin_DrawVisualizerHoriz(WINDOW *win, int left, int x, int y, int width, int height){
}

void VisualizerWin_DrawVisualizerVert(WINDOW *win ,int down, int x, int y, int width, int height){
	
	int len = 0;
	short *samples = Pulse_GetCurrentSamples(&len);
	if(len < width * 2) return;

	float lastSample = -1;

	int k;
	for(k = 0; k < width; k++){

		float sample = samples[k*2] + (0xFFFF/2);
		sample /= (float)(0xFFFF / height);
		sample = round(sample);
		if(sample > height) sample = height;

		if(lastSample == -1) lastSample = sample;
		if(sample > lastSample) sample = ++lastSample;
		if(sample < lastSample) sample = --lastSample;


		wattron(win, COLOR_PAIR(VISUALIZER_BACKGROUND_COLOR_PAIR));

		int f;
		for(f = y; f < y+height; f++)
			mvwaddch(win, f, x + k,' ');
		
		int color = fabs(sample-(height/2)) > 5 ? 5 : fabs(sample-(height/2));

		wattron(win, COLOR_PAIR(CYAN_BLACK_COLOR_PAIR+color));
		mvwaddch(win, y + sample, x + k, ACS_BLOCK);
	}
}

void VisualizerWin_UpdateDraw(WINDOW *win){
	VisualizerWin_DrawVisualizerVert(win, 0, 0, 0, COLS, LINES);
}