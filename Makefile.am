AUTOMAKE_OPTIONS = foreign

SUBDIRS = sayleen po doc

YYMMDD = `date +%Y%m%d`

EXTRA_DIST = ABOUT-NLS

ACLOCAL_AMFLAGS = -I m4

.version:
	-rm $(srcdir)/sayleen/version.h
	sed -e "s/zVERSION_IDz/$(YYMMDD)/g" $(srcdir)/sayleen/version.h.in > $(srcdir)/sayleen/version.h
	autoconf
