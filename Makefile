# Makefile for yahtzee

# Definitions for the game logic, and the dumb terminal interface.

CC = gcc
CFLAGS = -Wall -Wextra -Os
LDFLAGS = -Wall -Wextra -s
LOADLIBES =
OBJLIST = yahtzee.o gen.o scoring.o io.o iotext.o

# Definitions for the curses interface.
# Comment out this section to remove curses support.

CFLAGS += -DINCLUDE_CURSES
LOADLIBES += -lncurses
OBJLIST += iocurses.o

# Definitions for the SDL interface.
# Comment out this section to remove SDL support.

CFLAGS += -DINCLUDE_SDL $(shell sdl-config --cflags)
LOADLIBES += $(shell sdl-config --libs) -lSDL_ttf -lm
OBJLIST += iosdl.o sdldice.o sdlbutton.o sdlslots.o sdlhelp.o
# If fc-match doesn't exist on your system, edit this to provide
# explicit paths to the font files.
CFLAGS += -DFONT_MED_PATH='$(shell fc-match --format='"%{file}"' freesans)' \
    -DFONT_BOLD_PATH='$(shell fc-match --format='"%{file}"' freesans:bold)'

# Dependencies.

yahtzee: $(OBJLIST)

yahtzee.o: yahtzee.c yahtzee.h gen.h scoring.h io.h
gen.o: gen.c gen.h
scoring.o: scoring.c scoring.h yahtzee.h
io.o: io.c io.h iotext.h iocurses.h iosdl.h
iotext.o: iotext.c iotext.h yahtzee.h gen.h
iocurses.o: iocurses.c iocurses.h yahtzee.h gen.h
iosdl.o: iosdl.c iosdl.h gen.h yahtzee.h iosdlctl.h
sdldice.o: sdldice.c iosdlctl.h yahtzee.h gen.h
sdlbutton.o: sdlbutton.c iosdlctl.h yahtzee.h gen.h
sdlslots.o: sdlslots.c iosdlctl.h yahtzee.h gen.h
sdlhelp.o: sdlhelp.c iosdlctl.h yahtzee.h gen.h

clean:
	rm -f yahtzee $(OBJLIST)
