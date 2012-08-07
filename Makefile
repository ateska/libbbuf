SUBDIRS=src
CLEAN_SUBDIRS=test

########

all: $(SUBDIRS)

test: all
	@echo "Building tests"
	@$(MAKE) -C test all

########

top_builddir=.
include ${top_builddir}/build/make.rules
