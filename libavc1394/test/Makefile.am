MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = dvcont.1
bin_PROGRAMS = dvcont mkrfc2734
man_MANS = dvcont.1

noinst_PROGRAMS = romtest plugctl setrom avc_vcr plugreport

dvcont_SOURCES = dvcont.c
dvcont_LDADD = ../librom1394/librom1394.la ../libavc1394/libavc1394.la -lraw1394

romtest_SOURCES = romtest.c
romtest_LDADD = ../librom1394/librom1394.la -lraw1394

mkrfc2734_SOURCES = mkrfc2734.c
mkrfc2734_LDADD = ../librom1394/librom1394.la -lraw1394

plugctl_SOURCES = plugctl.c
plugctl_LDADD = ../libplug1394/libplug1394.la -lraw1394

setrom_SOURCES = setrom.c
setrom_LDADD = ../librom1394/librom1394.la -lraw1394

avc_vcr_SOURCES = avc_vcr.c
avc_vcr_LDADD = ../libavc1394/libavc1394.la -lraw1394

plugreport_SOURCES = plugreport.c
plugreport_LDADD = ../libplug1394/libplug1394.la -lraw1394

