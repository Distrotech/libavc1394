MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = dvcont.1
bin_PROGRAMS = dvcont
man_MANS = dvcont.1

noinst_PROGRAMS = romtest

dvcont_SOURCES = dvcont.c
dvcont_LDADD = ../librom1394/librom1394.la ../libavc1394/libavc1394.la -lraw1394

romtest_SOURCES = romtest.c
romtest_LDADD = ../librom1394/librom1394.la -lraw1394