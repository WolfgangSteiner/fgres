PRE_FIX= /usr/local
BINDIR=		$(PRE_FIX)/bin
MANDIR=		$(PRE_FIX)/man/man
CC=		gcc -O2
INSTALL=	install

#{{{}}}
#{{{  usually not needed configuration on buffer size..:
# Reading and writing files is done in blocks of BLOCKSIZE(16384) bytes (or
# multiples of this value, if the search/replace patterns are very long).
# Up to BLOCKS(64) blocks of BLOCKSIZE bytes of text are hold in memory, while
# extending a file, so extensions with more than BLOCKS*BLOCKSIZE may use
# a temporary file.
# These constants can be redefined with:
#BLC_CPP=	-DBLOCKS=64
#BLS_CPP=	-DBLOCKSIZE=16384

# in case, you cannot use ftruncate, uncomment
#TRUNC_CPP=	-DNO_FTRUNCATE

# in case, you do not want to unlink a temporary file after opening,
# but on close, uncomment
#UNL_CPP=	-DDELAYED_CLOSE

# in case, your system does not have <sysexit.h>, uncomment
#SE_CPP=	-DNOSYSEXIT
#}}}
#{{{  known targets:
#   default               ==binary and manual page
#   all                   ==default
#   fgres                 ==generate the executable
#   manual                ==generate the manual page
#   fgres.1               ==manual
#   install               ==man_install and cmd_install
#   man_install           ==install manual page in $(MANDIR)/man1
#   cmd_install           ==install binary in $(BINDIR)
#   clean                 ==remove object files
#   clobber               ==remove all generated files
#}}}
#{{{  changes below this line should not be neccessary
all default:	fgres fgres.1

#{{{  headers,sources and objects are
HEADERS=	global.h patchlevel.h

SOURCES=	file_io.c main.c messages.c program.c

OBJECTS=	$(SOURCES:.c=.o)
#}}}
#{{{  cc and cpp flags
CPPFLAGS= $(TRUNC_CPP) $(UNL_CPP) $(BLC_CPP) $(BLS_CPP) $(SE_CPP)

CFLAGS=$(CPPFLAGS)
#}}}
#{{{  dependencies and create command
$(OBJECTS):	$(HEADERS) Makefile

fgres:	$(OBJECTS)
	$(CC) -o $@ $(OBJECTS)
#}}}
#{{{  MANPAGE rules
manual:		fgres.1

fgres.1:	MANPAGE man.fgres fgres
	./fgres -f man.fgres <MANPAGE >$@

man.fgres:	global.h messages.c patchlevel.h gen_man.fgres fgres Makefile
	cat global.h patchlevel.h | ./fgres -f gen_man.fgres >man.fgres.tmp
	cp man.fgres.tmp $@
	./fgres -f man.fgres.tmp <messages.c | ./fgres -f gen_man.fgres >>$@
	/bin/rm -f man.fgres.tmp
#}}}
#{{{  install and deinstall
install:	cmd_install man_install

cmd_install:	fgres
	$(INSTALL) fgres $(BINDIR)/fgres
	strip $(BINDIR)/fgres
	chmod 755 $(BINDIR)/fgres

man_install:	fgres.1
	$(INSTALL) fgres.1 $(MANDIR)/man1/fgres.1
	chmod 644 $(MANDIR)/man1/fgres.1

deinstall:
	rm -f $(BINDIR)/fgres $(MANDIR)/man1/fgres.1
#}}}
#{{{  clean and clobber
clean:
	rm -f $(OBJECTS) core*

very_clean:	clobber

clobber:	clean
	rm -f fgres* man.fgres*
#}}}
#}}}
