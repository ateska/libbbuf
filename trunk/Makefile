SUBDIRS=src
CLEAN_SUBDIRS=test

########

all: $(SUBDIRS)

install: all
	@echo "Installing into ${PREFIX}"
	@${INSTALL} -d ${PREFIX}/include
	@$(LIBTOOL) --tag CC --quiet --mode=install ${INSTALL} ${top_builddir}/include/*.h ${PREFIX}/include
	@${INSTALL} -d ${PREFIX}/lib
	@$(LIBTOOL) --tag CC --quiet --mode=install ${INSTALL} ${top_builddir}/src/libbbuf.la ${PREFIX}/lib

test: all
	@echo "Building tests"
	@$(MAKE) -C test all

########

top_builddir=.
include ${top_builddir}/build/make.rules
