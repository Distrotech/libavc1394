MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = libplug1394.la
libplug1394_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@  -lm
libplug1394_la_LIBADD = $(top_builddir)/common/raw1394util.lo
libplug1394_la_SOURCES = plug1394.c
pkginclude_HEADERS = plug1394.h
