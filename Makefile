CC=gcc

CFLAGS=-g -Wall -lm
CFLAGS ?= $(shell pkg-config --static --cflags ncurses)
CFLAGS ?= $(shell pkg-config --static --cflags libpulse)
CFLAGS ?= $(shell pkg-config --static --cflags mad-mainloop-glib)
CFLAGS ?= $(shell pkg-config --static --cflags mad)
CFLAGS ?= $(shell pkg-config --static --cflags libpng)
 
NCURSESLIBS = $(shell pkg-config --static --libs ncurses)
PULSELIBS   = $(shell pkg-config --static --libs libpulse)
PULSELIBS   = $(shell pkg-config --static --libs libpulse-mainloop-glib)
MADLIBS     = $(shell pkg-config --static --libs mad)
PNGLIBS     = $(shell pkg-config --static --libs libpng)
LUALIBS 	= $(shell lua-config --static)
FLACLIBS 	= $(shell pkg-config --static --libs flac)
XLIBS       = $(shell pkg-config --libs x11)
XLIBS      ?= $(shell pkg-config --libs libva-x11)
XLIBS      ?= $(shell pkg-config --libs x11-xcb)
OPENSSLIBS = $(shell pkg-config --libs openssl)

LDLIBS =-lm -static-libgcc -Wl,-Bstatic $(NCURSESLIBS) $(MADLIBS) -ljpeg $(PNGLIBS) $(FLACLIBS) -Wl,-Bdynamic $(PULSELIBS) \
$(OPENSSLIBS) $(XLIBS) $(LUALIBS)  

SOURCES=main.c pulse.c id3.c mp3.c ui.c utils.c browser_win.c visualizer_win.c x11.c config.c flac.c lastfm.c library_win.c player.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=player.exe

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDLIBS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@