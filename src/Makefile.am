## Automake template for Inform5 program.

include $(top_srcdir)/config/Makefile.inc

bin_PROGRAMS = inform5

inform5_SOURCES = asm.c express.c files.c inform.c inputs.c symbols.c \
tables.c zcode.c header.h

INC= ,$(DESTDIR)$(localincdir),$(DESTDIR)$(includedir)
INCDEF = -DInclude_Directory=\"$(INC)\"
INCLUDES = -D$(HOST) $(MODDEF) $(INCDEF)

PROGRAM = $(DESTDIR)$(bindir)/inform5

install-exec-hook:
if CYGWIN
#	Don't bother keeping old versions under Cygwin.
else
	mv $(PROGRAM) $(PROGRAM)-$(INFORM_VERSION)
	$(LN_S) $(PROGRAM)-$(INFORM_VERSION) $(PROGRAM)
endif

uninstall-hook:
if CYGWIN
#	Ditto.
else
	- chmod u+w $(DESTDIR)$(bindir)
	rm -f $(PROGRAM)-$(INFORM_VERSION)
endif
