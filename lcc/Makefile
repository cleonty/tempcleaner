# Wedit Makefile for project tempcleaner
SRCDIR=c:\lcc\projects\tempcleaner
CFLAGS=-I"C:\lcc\include"  -g2 
CC=$(LCCROOT)\bin\lcc.exe
LINKER=$(LCCROOT)\bin\lcclnk.exe
OBJS=\
	tempcleaner.obj \
	tempcleaner.res

LIBS=
EXE=tempcleaner.exe

$(EXE):	$(OBJS) Makefile
	$(LINKER)  -subsystem windows -o $(SRCDIR)\lcc\tempcleaner.exe $(OBJS) $(LIBS)

# Build tempcleaner.c
TEMPCLEANER_C=\
	c:\lcc\include\_syslist.h\
	c:\lcc\include\_syslist.h\
	c:\lcc\include\safelib.h\
	c:\lcc\include\string.h\
	c:\lcc\include\safelib.h\
	c:\lcc\include\win.h\
	c:\lcc\include\limits.h\
	c:\lcc\include\stdarg.h\
	c:\lcc\include\ctype.h\
	c:\lcc\include\basetsd.h\
	$(SRCDIR)\tempcleanerres.h\

tempcleaner.obj: $(TEMPCLEANER_C) $(SRCDIR)\tempcleaner.c
	$(CC) -c $(CFLAGS) $(SRCDIR)\tempcleaner.c

# Build tempcleaner.res
TEMPCLEANER_RC=\
	$(SRCDIR)\tempcleanerres.h\

tempcleaner.res:	$(TEMPCLEANER_RC) $(SRCDIR)\tempcleaner.rc
	$(LCCROOT)\bin\lrc.exe -I$(SRCDIR) -I"C:\lcc\include"  $(SRCDIR)\tempcleaner.rc

link:
	$(LINKER)  -subsystem windows -o $(SRCDIR)\lcc\tempcleaner.exe $(OBJS) $(LIBS)

clean:
	del $(OBJS) tempcleaner.exe
