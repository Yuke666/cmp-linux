CC=gcc

CFLAGS=-g -Wall -lm
CFLAGS ?= $(shell pkg-config --cflags ncurses)
CFLAGS ?= $(shell pkg-config --cflags libpulse)
CFLAGS ?= $(shell pkg-config --cflags mad-mainloop-glib)
CFLAGS ?= $(shell pkg-config --cflags mad)
CFLAGS ?= $(shell pkg-config --cflags libpng)
CFLAGS ?= $(shell pkg-config --cflags lua)
 
LUALIBS 	= $(shell pkg-config --libs lua)
NCURSESLIBS = $(shell pkg-config --libs ncurses)
PULSELIBS   = $(shell pkg-config --libs libpulse)
PULSELIBS   = $(shell pkg-config --libs libpulse-mainloop-glib)
MADLIBS     = $(shell pkg-config --libs mad)
PNGLIBS     = $(shell pkg-config --libs libpng)
FLACLIBS 	= $(shell pkg-config --libs flac)
XLIBS       = $(shell pkg-config --libs x11)
XLIBS      ?= $(shell pkg-config --libs libva-x11)
XLIBS      ?= $(shell pkg-config --libs x11-xcb)
OPENSSLIBS = $(shell pkg-config --libs openssl)

LDLIBS =-lm $(NCURSESLIBS) $(MADLIBS) -ljpeg $(PNGLIBS) $(FLACLIBS) $(PULSELIBS) \
$(OPENSSLIBS) $(XLIBS) $(LUALIBS)  

SOURCES=main.c pulse.c id3.c mp3.c ui.c utils.c browser_win.c visualizer_win.c x11.c config.c flac.c lastfm.c library_win.c player.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=player

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDLIBS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm *.o