## Automake template for Inform5 libraries.

include $(top_srcdir)/config/Makefile.inc

INCLUDES = grammar.h parser.h verblib.h

EXTRA_NAMES = Grammar.h Parser.h Verblib.h

EXTRA_DIST = $(INCLUDES)

ETAGS_ARGS = $(ETAGS_INFORM) $(INCLUDES)

TAGS_DEPENDENCIES = $(INCLUDES)

if CYGWIN
install-data-hook: add-localdir
uninstall-local: remove-localdir
else
all-local: add-local-names
install-data-hook: add-localdir add-names
uninstall-local: remove-localdir remove-names
endif

add-localdir:
	$(mkinstalldirs) $(DESTDIR)$(localmoddir)

remove-localdir:
	rmdir $(DESTDIR)$(localmoddir)

add-local-names:
	@- cd $(top_builddir)/lib;					\
	for file in $(EXTRA_NAMES); do					\
	   realfile=`echo $$file | tr '[A-Z]' '[a-z]'`;			\
	   test -r $$file || $(LN_S) $(srcdir)/$$realfile $$file;	\
	done

add-names:
	@ cd $(DESTDIR)$(moduledir);					\
	for file in $(EXTRA_NAMES); do					\
	    if test ! -e $$file -o -L $$file; then			\
		realfile=`echo $$file | tr '[A-Z]' '[a-z]'`;		\
		echo " linking $$file to $$realfile";			\
		rm -f $$file;						\
		$(LN_S) $$realfile $$file;				\
	    fi								\
	done

remove-names:
	@ for file in $(EXTRA_NAMES); do				\
	    if test -L $$file; then					\
		echo " unlinking $$file";				\
		rm -f $(DESTDIR)$(moduledir)/$$file;			\
	    fi								\
	done

