SHELL=/bin/bash
MACHINE=$(shell uname -s)

# if PREFIX empty, defaults to home directory
# otherwise, use value passed by command line
# (should help with rpm build)
ifeq ($(PREFIX),)
	PREFIX=build/sage
endif

# These are built before sage
preprojects = \
QUANTA \
sam \
tools

# These are built after sage
subprojects = \
apps/appLauncher \
apps/DecklinkCapture \
apps/dim \
apps/dxt \
apps/FileViewer \
apps/Image3D \
apps/ImageFlip \
apps/ImageS3D \
apps/mplayer \
apps/qshare \
apps/sageProxy \
apps/ui \
apps/ultragrid-1.1 \
apps/vnc \
apps/webcam

USE_EXAMPLES=1
ifeq ($(USE_EXAMPLES), 1)
subprojects += \
examples/atlantis \
examples/atlantis-fbo \
examples/bitplay \
examples/checker \
examples/render \
examples/yuv
endif

all: $(subprojects)
# dim

.PHONY: $(preprojects) $(subprojects) sage dim install clean

################################################################################
# apps depend on sage
$(subprojects): sage
	$(MAKE) -C src/$@

# sage depends on quanta and sam
sage: $(preprojects)
	$(MAKE) -C src/$@

# quanta and sam
$(preprojects):
	$(MAKE) -C src/$@

install: all
# copy OS environment settings
	mkdir -p $(PREFIX)/etc/profile.d
	mkdir -p $(PREFIX)/etc/ld.so.conf.d
	cp etc/sage.csh $(PREFIX)/etc/profile.d
	cp etc/sage.sh $(PREFIX)/etc/profile.d
	cp etc/sage.conf $(PREFIX)/etc/ld.so.conf.d
# copy binaries
	mkdir -p $(PREFIX)/bin
	cp -ruf bin/* $(PREFIX)/bin
# copy libraries
ifeq ($(MACHINE), Darwin)
	mkdir -p $(PREFIX)/lib
	cp lib/libquanta.dylib $(PREFIX)/lib
	cp lib/libsail.dylib $(PREFIX)/lib
else
	mkdir -p $(PREFIX)/lib64
	cp lib/libquanta.so $(PREFIX)/lib64
	cp lib/libsail.so $(PREFIX)/lib64
	cp lib/libsac.a $(PREFIX)/lib64
endif
# copy includes
	mkdir -p $(PREFIX)/include
	cp src/sage/*.h $(PREFIX)/include
	cp src/sam/src/*.h $(PREFIX)/include
	cp src/sam/src/client/*.h $(PREFIX)/include
# copy configuration files
	mkdir -p $(PREFIX)/sageConfig
	cp -ruf sageConfig/* $(PREFIX)/sageConfig
# copy images
	mkdir -p $(PREFIX)/images
	cp -ruf images/* $(PREFIX)/images
# copy sample files
	mkdir -p $(PREFIX)/resources
	cp -ruf resources/* $(PREFIX)/resources
# copy fonts
	mkdir -p $(PREFIX)/fonts
	cp fonts/*.ttf $(PREFIX)/fonts
# copy makefile
	cp config.mk $(PREFIX)/

tarball: install
	tar -zcvf build/sage.tar.gz -C $(PREFIX) .

clean:
	@for i in `echo $(preprojects)`; do cd src/$$i; $(MAKE) clean; cd - ; done
	cd src/sage; $(MAKE) clean; cd -
	@for i in `echo $(subprojects)`; do cd src/$$i; $(MAKE) clean; cd - ; done

distclean:
	rm -rf obj
	rm -rf src/apps/ultragrid-1.1/build
	@for i in `echo $(preprojects)`; do cd src/$$i; $(MAKE) distclean; cd - ; done
	cd src/sage; $(MAKE) distclean; cd -
	@for i in `echo $(subprojects)`; do cd src/$$i; $(MAKE) distclean; cd - ; done
