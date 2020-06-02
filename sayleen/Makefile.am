EXTRA_DIST = version.h.in sayleen

rcpconvincludedir = $(includedir)

bin_PROGRAMS     = rcpconv smfplay
lib_LTLIBRARIES  = librcpconv.la
bin_SCRIPTS      = sayleen

librcpconv_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

librcpconv_la_SOURCES = rcp_functions.c rcp_functions.h \
                        rcptomid.c smf.c rcp.h smf.h version.h
rcpconv_SOURCES = rcpconv_main.c gettext_wrapper.h \
                  rcpfile.c rcpconv.c rcpconv.h \
                  mididev.c rcpplay.c mididev.h
rcpconv_DEPENDENCIES = version.h .libs/librcpconv.la

smfplay_SOURCES = smfplay_main.c smffile.c smfplay.c \
                  smfplay.h smf.h version.h \
                  mididev.c mididev.h

EXTRA_rcpconv_SOURCES = getopt.c getopt_long.c netbsdgetopt.h
EXTRA_smfplay_SOURCES = getopt.c getopt_long.c netbsdgetopt.h

rcpconv_DEPENDENCIES = @GETOPT_OBJ@
smfplay_DEPENDENCIES = @GETOPT_OBJ@

rcpconvinclude_HEADERS = \
                        rcpconv.h

OMIT_DEPENDENCIES = rcddef.h

localedir = $(datadir)/locale

INCLUDES = -I./ -I../ -I$(top_srcdir)/intl -DLOCALEDIR=\"$(localedir)\"

rcpconv_LDFLAGS = -rpath $(libdir):$(pkglibdir)

rcpconv_LDADD = -L$(DIST_ROOT)/sayleen -L$(libdir) -L$(prefix)/lib -lrcpconv @INTLLIBS@ @LIBS@ @GETOPT_OBJ@

smfplay_LDADD = @INTLLIBS@ @LIBS@ @GETOPT_OBJ@

install-exec-hook:
	ln -sf $(bindir)/rcpconv $(bindir)/rcpplay
