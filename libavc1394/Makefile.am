SUBDIRS = common libavc1394 librom1394 test
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config.h.in \
	stamp-h.in
install-exec-hook:
	@if [ ! -c /dev/raw1394 ]; then \
		echo; \
		echo "********************************************"; \
		echo "Please install libraw1394 before installing"; \
		echo "the 1394 AV/C Library"; \
		echo "********************************************"; \
		echo; \
	fi

