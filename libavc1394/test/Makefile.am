MAINTAINERCLEANFILES = Makefile.in
bin_PROGRAMS = dvcont
noinst_PROGRAMS = romtest
dvcont_SOURCES = dvcont.c
dvcont_LDADD = $(top_builddir)/libavc1394/libavc1394.la -lraw1394
romtest_SOURCE = romtest.c
romtest_LDADD = $(top_builddir)/librom1394/librom1394.la -lraw1394