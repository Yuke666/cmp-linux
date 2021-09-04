#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "x11.h"

static Display *display;
static Window window, ourWindow, root, *children, parent;
static GC gc;
static int x11Init = 0;

void X11_Init(){
    display = XOpenDisplay(0);
    char *id;
    int revert;
    unsigned int nchildren;

    if((id = getenv("WINDOWID")) != NULL){
        window = (Window)atoi(id);
    } else {
        XGetInputFocus(display, &window, &revert);
    }

    if(!window) return;

    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);

    int width = 0, height = 0;

    while(1){
        Window p_window;
        XQueryTree(display, window, &root, &parent, &children, &nchildren);
        p_window = window;
        int i;
        for(i = 0; i < nchildren; i++){
            XGetWindowAttributes(display, children[i], &attr);
            if(attr.width > width && attr.height > height){
                width = attr.width;
                height = attr.height;
                window = children[i];
            }
        }

        if(p_window == window) break;
    }

    if(width == 1 && height == 1)
        window = parent;

    unsigned long windowMask;
    XSetWindowAttributes winAttrib; 
            
    windowMask = CWBackPixel | CWBorderPixel;   
    winAttrib.border_pixel = BlackPixel (display, 0);
    winAttrib.background_pixel = BlackPixel (display, 0);
    winAttrib.override_redirect = 0;

    ourWindow = XCreateWindow(display, window, attr.x, attr.y, attr.width, attr.height, attr.border_width, attr.depth, attr.class, 
        attr.visual, windowMask, &winAttrib );

    gc = XCreateGC(display, ourWindow, 0, NULL);


    x11Init = 1;
}

void X11_Close(){
    if(!x11Init) return;
    XFreeGC(display,gc);
    XCloseDisplay(display);
    x11Init = 0;
}

int X11_GetGlobalKeys(int *ret){
    char keys[32];
    XQueryKeymap(display, keys);
    int k, returnOne = 0;
    for(k = 0; k < 32; k++){ 
        if(keys[k]) {
            int f;
            for(f = 0; f < 8; f++){
                if(keys[k] & 0x01 << f){
                    XKeyEvent keyEvent;
                    keyEvent.keycode = (k*8)+f;
                    keyEvent.display = display;
                    char *str = XKeysymToString(XLookupKeysym(&keyEvent, 0));
                    if(strlen(str) == 1) ret[(k*8)+f] = (int)str[0];
                    else if(strcmp(str, "equal") == 0) ret[(k*8)+f] = '=';
                    else if(strcmp(str, "minus") == 0) ret[(k*8)+f] = '-';
                    else if(strcmp(str, "Control_L") == 0) ret[(k*8)+f] = X11_CONTROL_KEY;
                    else if(strcmp(str, "Control_R") == 0) ret[(k*8)+f] = X11_CONTROL_KEY;
                    else if(strcmp(str, "Alt_L") == 0) ret[(k*8)+f] = X11_ALT_KEY;
                    else if(strcmp(str, "Alt_R") == 0) ret[(k*8)+f] = X11_ALT_KEY;
                    else if(strcmp(str, "Super_L") == 0) ret[(k*8)+f] = X11_SUPER_KEY;
                    else if(strcmp(str, "Super_R") == 0) ret[(k*8)+f] = X11_SUPER_KEY;
                    returnOne = 1;
                }
            }
        }
    }
    return returnOne;
}

void X11_WithdrawWindow(){
    if(!x11Init) return;
    XWithdrawWindow(display, ourWindow, DefaultScreen(display));
    XSync(display,False);
}

void X11_DrawSongImage(SongImage *img, int xPos, int yPos, int drawWidth, int drawHeight){

    if(!x11Init || !ourWindow || !img->pixels) return;

    XWindowAttributes attr;
    XMapWindow(display, ourWindow);

    XGetWindowAttributes(display, window, &attr);

    xPos *= (attr.width/COLS);
    yPos *= (attr.height/LINES);
    drawWidth *= (attr.width/COLS);
    drawHeight *= (attr.height/LINES);
    if(drawWidth <= 0) drawWidth = 1;
    if(drawHeight <= 0) drawHeight = 1;

    XMoveResizeWindow(display, ourWindow, xPos, yPos, drawWidth, drawHeight);

    float xPlus = (float)img->width / (float)drawWidth;
    float yPlus = (float)img->height / (float)drawHeight;
    char *pixels = (char*)malloc(sizeof(char) * img->width/xPlus * img->height/yPlus * 3);

    int pixelIndex = 0;
    int xround, yround;

    float x, y;
    for(y = 0; y < img->height; y+=yPlus){
        for(x = 0; x < img->width; x+=xPlus){

            if(round(x/xPlus) >= drawWidth) continue;

            yround = round(y)*img->width;
            xround = round(x);
            if(xround > img->width) break;

            char r = img->pixels[((yround + xround)*img->channels)  ] & 0xFF;
            char g = img->pixels[((yround + xround)*img->channels)+1] & 0xFF;
            char b = img->pixels[((yround + xround)*img->channels)+2] & 0xFF;

            if(pixelIndex+3 > img->width/xPlus * img->height/yPlus * 3) goto out;
            pixels[pixelIndex++] = r;
            pixels[pixelIndex++] = g;
            pixels[pixelIndex++] = b;
        }
    }
    
    out:

    if(attr.depth >= 24){

        int *newBuf = (int *)malloc(sizeof(int)*drawWidth*drawHeight);
        int newbufIndex = 0;
        int k;
        for(k = 0; k < drawWidth*drawHeight*3; k+=3){
            int r = (pixels[k]   & 0xFF) * (attr.visual->red_mask   / 255);
            int g = (pixels[k+1] & 0xFF) * (attr.visual->green_mask / 255);
            int b = (pixels[k+2] & 0xFF) * (attr.visual->blue_mask  / 255);
            r &= attr.visual->red_mask;
            g &= attr.visual->green_mask;
            b &= attr.visual->blue_mask;

            newBuf[newbufIndex++] = r | g | b;
        }

        XImage *xi = XCreateImage(display, attr.visual, 
            attr.depth, ZPixmap, 0, (char *)newBuf, drawWidth, drawHeight, 32, 0);

        XInitImage(xi);
        XPutImage(display, ourWindow, gc, xi, 0, 0, 0, 0, drawWidth, drawHeight);
        XSync(display, False);
        XDestroyImage(xi);
    }

    free(pixels);
}