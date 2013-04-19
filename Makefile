CC = gcc
CFLAGS = -Wall -W -O
LDFLAGS = -Wall -W -s
LOADLIBES =
OBJLIST = yahtzee.o gen.o scoring.o io.o iotext.o

# Comment out this section to remove support for the curses interface.
CFLAGS += -DINCLUDE_CURSES
LOADLIBES += -lncurses
OBJLIST += iocurses.o

# Comment out this section to remove support for the SDL interface.
CFLAGS += -DINCLUDE_SDL $(shell sdl-config --cflags)
LOADLIBES += $(shell sdl-config --libs) -lSDL_ttf -lm
OBJLIST += iosdl.o sdldice.o sdlbutton.o sdlslots.o sdlhelp.o

# Point this to the system directory containing the FreeFont fonts.
#CFLAGS += '-DFONT_DIR="/usr/share/fonts/truetype/freefont"'


yahtzee: $(OBJLIST)

yahtzee.o: yahtzee.c yahtzee.h gen.h scoring.h io.h
gen.o: gen.c gen.h
scoring.o: scoring.c scoring.h yahtzee.h
io.o: io.c io.h iocurses.h iosdl.h
iotext.o: iotext.c iotext.h yahtzee.h gen.h
iocurses.o: iocurses.c iocurses.h yahtzee.h gen.h
iosdl.o: iosdl.c iosdl.h gen.h yahtzee.h iosdlctl.h
sdldice.o: sdldice.c iosdlctl.h yahtzee.h
sdlbutton.o: sdlbutton.c iosdlctl.h yahtzee.h gen.h
sdlslots.o: sdlslots.c iosdlctl.h yahtzee.h gen.h
sdlhelp.o: sdlhelp.c iosdlctl.h yahtzee.h gen.h

clean:
	rm -f yahtzee $(OBJLIST)
