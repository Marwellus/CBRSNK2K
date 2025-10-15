# CyberSnake 2000 Turbo Makefile for Watcom C/C++ 11.0
# Target: 32-bit DOS4GW Protected Mode Executable

# Compiler and tool definitions
CC = wcc386
CPP = wpp386
LINK = wlink
RM = rm

# Directories
SRCDIR = src
INCDIR = inc
OBJDIR = bin/obj
BINDIR = bin
AWELIB = awe/pawe32.lib
GUSLIB0 = gus/ultra0wc.lib
GUSLIB1 = gus/ultra1wc.lib
GUSCODC = gus/ult16wc.lib
GUSUTIL = gus/utilwc.lib

# Target executable
TARGET = $(BINDIR)/snake.exe

# C Compiler flags
# CFLAGS = -mf -3r -ox -zq -bt=dos -i=$(INCDIR)
CFLAGS = -onatx -oh -oi -ei -4 -fp3 -zq -bt=dos -i=$(INCDIR)
# CFLAGS = -mf -d0 -od -s -zq -bt=dos -i=$(INCDIR) // debug

# C++ Compiler flags  
# CPPFLAGS = -mf -3r -ox -d0 -zq -bt=dos -i=$(INCDIR)
CPPFLAGS = -onatx -oh -oi+ -ei -4 -fp3 -zq -bt=dos -i=$(INCDIR)
# CPPFLAGS = -mf -d0 -od -s -zq -bt=dos -i=$(INCDIR) // debug

# Linker flags
LFLAGS = FORMAT dos4g &
         OPTION quiet &
         OPTION map &
         OPTION stack=64k

# Object files with proper paths
CPPOBJS = $(OBJDIR)/main.obj &
          $(OBJDIR)/game.obj &
          $(OBJDIR)/draw.obj &
          $(OBJDIR)/video.obj &
          $(OBJDIR)/field.obj &
          $(OBJDIR)/snake.obj &
          $(OBJDIR)/cater.obj &
          $(OBJDIR)/boss.obj &
          $(OBJDIR)/font.obj &
          $(OBJDIR)/sound.obj &
			 $(OBJDIR)/music.obj

COBJS = $(OBJDIR)/timer.obj &
        $(OBJDIR)/memory.obj &
        $(OBJDIR)/fileio.obj &
		  $(OBJDIR)/dsp.obj &
		  $(OBJDIR)/opl.obj &
		  $(OBJDIR)/loadgif.obj &
		  $(OBJDIR)/awe32drv.obj &
		  $(OBJDIR)/gusdrv.obj &
 		  $(OBJDIR)/midi.obj &
		  $(OBJDIR)/miditmr.obj &
		  $(OBJDIR)/scroller.obj &
		  $(OBJDIR)/logger.obj &
		  $(OBJDIR)/loadgif.obj

OBJS = $(CPPOBJS) $(COBJS)

# Default target
all: $(TARGET)

# Create directories if they don't exist
$(OBJDIR):
	@if not exist $(OBJDIR) mkdir $(OBJDIR)

$(BINDIR):
	@if not exist $(BINDIR) mkdir $(BINDIR)

# Link target
$(TARGET): $(BINDIR) $(OBJS)
	@echo Creating response file
	@echo SYSTEM dos4g > link.rsp
	@echo OPTION quiet >> link.rsp
	@echo OPTION map >> link.rsp
	@echo OPTION stack=64k >> link.rsp
	@echo DEBUG all >> link.rsp
	@echo NAME $(TARGET) >> link.rsp
	@echo FILE $(OBJDIR)/main.obj >> link.rsp
	@echo FILE $(OBJDIR)/game.obj >> link.rsp
	@echo FILE $(OBJDIR)/draw.obj >> link.rsp
	@echo FILE $(OBJDIR)/video.obj >> link.rsp
	@echo FILE $(OBJDIR)/field.obj >> link.rsp
	@echo FILE $(OBJDIR)/snake.obj >> link.rsp
	@echo FILE $(OBJDIR)/cater.obj >> link.rsp
	@echo FILE $(OBJDIR)/boss.obj >> link.rsp
	@echo FILE $(OBJDIR)/font.obj >> link.rsp
	@echo FILE $(OBJDIR)/sound.obj >> link.rsp
	@echo FILE $(OBJDIR)/timer.obj >> link.rsp
	@echo FILE $(OBJDIR)/memory.obj >> link.rsp
	@echo FILE $(OBJDIR)/fileio.obj >> link.rsp
	@echo FILE $(OBJDIR)/dsp.obj >> link.rsp
	@echo FILE $(OBJDIR)/opl.obj >> link.rsp
	@echo FILE $(OBJDIR)/awe32drv.obj >> link.rsp
	@echo FILE $(OBJDIR)/gusdrv.obj >> link.rsp
	@echo FILE $(OBJDIR)/midi.obj >> link.rsp
	@echo FILE $(OBJDIR)/miditmr.obj >> link.rsp
	@echo FILE $(OBJDIR)/scroller.obj >> link.rsp
	@echo FILE $(OBJDIR)/music.obj >> link.rsp
	@echo FILE $(OBJDIR)/logger.obj >> link.rsp
	@echo FILE $(OBJDIR)/loadgif.obj >> link.rsp
	@echo LIBRARY $(AWELIB) >> link.rsp
	@echo LIBRARY $(GUSLIB0) >> link.rsp
	@echo LIBRARY $(GUSLIB1) >> link.rsp
	@echo LIBRARY $(GUSCODC) >> link.rsp
	@echo LIBRARY $(GUSUTIL) >> link.rsp
	@echo Linking $@...
	$(LINK) @link.rsp
	@$(RM) link.rsp

# Explicit rules for C++ files
$(OBJDIR)/main.obj: $(SRCDIR)/main.cpp
	@echo Compiling main.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/main.cpp

$(OBJDIR)/game.obj: $(SRCDIR)/game.cpp
	@echo Compiling game.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/game.cpp

$(OBJDIR)/draw.obj: $(SRCDIR)/draw.cpp
	@echo Compiling draw.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/draw.cpp

$(OBJDIR)/video.obj: $(SRCDIR)/video.cpp
	@echo Compiling video.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/video.cpp

$(OBJDIR)/field.obj: $(SRCDIR)/field.cpp
	@echo Compiling field.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/field.cpp

$(OBJDIR)/snake.obj: $(SRCDIR)/snake.cpp
	@echo Compiling snake.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/snake.cpp

$(OBJDIR)/cater.obj: $(SRCDIR)/cater.cpp
	@echo Compiling cater.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/cater.cpp

$(OBJDIR)/boss.obj: $(SRCDIR)/boss.cpp
	@echo Compiling boss.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/boss.cpp

$(OBJDIR)/font.obj: $(SRCDIR)/font.cpp
	@echo Compiling font.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/font.cpp

$(OBJDIR)/sound.obj: $(SRCDIR)/sound.cpp
	@echo Compiling sound.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/sound.cpp

$(OBJDIR)/music.obj: $(SRCDIR)/music.cpp
	@echo Compiling music.cpp...
	$(CPP) $(CPPFLAGS) -fo=$@ $(SRCDIR)/music.cpp

# Explicit rules for C files
$(OBJDIR)/timer.obj: $(SRCDIR)/timer.c
	@echo Compiling timer.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/timer.c

$(OBJDIR)/memory.obj: $(SRCDIR)/memory.c
	@echo Compiling memory.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/memory.c

$(OBJDIR)/fileio.obj: $(SRCDIR)/fileio.c
	@echo Compiling fileio.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/fileio.c

$(OBJDIR)/dsp.obj: $(SRCDIR)/dsp.c
	@echo Compiling dsp.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/dsp.c

$(OBJDIR)/opl.obj: $(SRCDIR)/opl.c
	@echo Compiling opl.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/opl.c

$(OBJDIR)/awe32drv.obj: awe/awe32drv.c
	@echo Compiling awe32drv.c...
	$(CC) $(CFLAGS) -fo=$@ awe/awe32drv.c

$(OBJDIR)/gusdrv.obj: gus/gusdrv.c
	@echo Compiling gusdrv.c...
	$(CC) $(CFLAGS) -fo=$@ gus/gusdrv.c

$(OBJDIR)/midi.obj: $(SRCDIR)/midi.c
	@echo Compiling midi.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/midi.c

$(OBJDIR)/miditmr.obj: $(SRCDIR)/miditmr.c
	@echo Compiling miditmr.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/miditmr.c

$(OBJDIR)/scroller.obj: $(SRCDIR)/scroller.c
	@echo Compiling scroller.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/scroller.c

$(OBJDIR)/logger.obj: $(SRCDIR)/logger.c
	@echo Compiling logger.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/logger.c

$(OBJDIR)/loadgif.obj: $(SRCDIR)/loadgif.c
	@echo Compiling loadgif.c...
	$(CC) $(CFLAGS) -fo=$@ $(SRCDIR)/loadgif.c

# Clean build
clean:
	@echo Cleaning up...
	@if exist $(OBJDIR)/*.obj $(RM) $(OBJDIR)/*.obj
	@if exist $(TARGET) $(RM) $(TARGET)
	@if exist *.map $(RM) *.map
	@if exist *.err $(RM) *.err
	@if exist link.rsp $(RM) link.rsp

# Run the game
run: $(TARGET)
	$(TARGET)

# Force rebuild
rebuild: clean all

# Dependencies for header changes
$(OBJDIR)/main.obj: $(INCDIR)/video.hpp $(INCDIR)/game.hpp $(INCDIR)/timer.h 
$(OBJDIR)/game.obj: $(INCDIR)/game.hpp $(INCDIR)/draw.hpp $(INCDIR)/sound.hpp $(INCDIR)/music.hpp $(INCDIR)/creaturs.hpp $(INCDIR)/field.hpp $(INCDIR)/timer.h $(INCDIR)/dsp.h
$(OBJDIR)/draw.obj: $(INCDIR)/draw.hpp $(INCDIR)/video.hpp $(INCDIR)/font.hpp $(INCDIR)/memory.h $(INCDIR)/fileio.h $(INCDIR)/scroller.h $(INCDIR)/loadgif.h
$(OBJDIR)/field.obj: $(INCDIR)/field.hpp $(INCDIR)/sound.hpp $(INCDIR)/video.hpp $(INCDIR)/draw.hpp $(INCDIR)/timer.h
$(OBJDIR)/snake.obj: $(INCDIR)/creaturs.hpp $(INCDIR)/draw.hpp $(INCDIR)/sound.hpp $(INCDIR)/field.hpp
$(OBJDIR)/cater.obj: $(INCDIR)/creaturs.hpp $(INCDIR)/draw.hpp $(INCDIR)/sound.hpp $(INCDIR)/field.hpp
$(OBJDIR)/boss.obj: $(INCDIR)/creaturs.hpp $(INCDIR)/draw.hpp $(INCDIR)/sound.hpp $(INCDIR)/field.hpp
$(OBJDIR)/video.obj: $(INCDIR)/video.hpp $(INCDIR)/memory.h
$(OBJDIR)/timer.obj: $(INCDIR)/timer.h $(INCDIR)/miditmr.h
$(OBJDIR)/midi.obj: $(INCDIR)/midi.h $(INCDIR)/miditmr.h
$(OBJDIR)/music.obj: $(INCDIR)/music.hpp awe/awe32drv.h gus/gusdrv.h
$(OBJDIR)/fileio.obj: $(INCDIR)/fileio.h $(INCDIR)/memory.h
$(OBJDIR)/scroller.obj: $(INCDIR)/scroller.h $(INCDIR)/memory.h
$(OBJDIR)/sound.obj: $(INCDIR)/sound.hpp $(INCDIR)/opl.h
$(OBJDIR)/memory.obj: $(INCDIR)/memory.h
$(OBJDIR)/miditmr.obj: $(INCDIR)/miditmr.h
$(OBJDIR)/dsp.obj: $(INCDIR)/dsp.h
$(OBJDIR)/opl.obj: $(INCDIR)/opl.h
$(OBJDIR)/awe32drv.obj: awe/awe32drv.h
$(OBJDIR)/gusdrv.obj: gus/gusdrv.h gus/libh/patch.h
$(OBJDIR)/font.obj: $(INCDIR)/font.hpp
$(OBJDIR)/loadgif.obj: $(INCDIR)/loadgif.h
$(OBJDIR)/logger.obj: $(INCDIR)/logger.h