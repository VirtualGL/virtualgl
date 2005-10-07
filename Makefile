all: rr mesademos diags samples
ALL: dist mesademos

.PHONY: rr util jpeg mesademos diags clean dist samples test

rr: util jpeg
jpeg: util
samples: rr

rr util jpeg mesademos diags samples:
	cd $@; $(MAKE); cd ..

clean:
	cd rr; $(MAKE) clean; cd ..; \
	cd util; $(MAKE) clean; cd ..; \
	cd jpeg; $(MAKE) clean; cd ..; \
	cd mesademos; $(MAKE) clean; cd ..; \
	cd diags; $(MAKE) clean; cd ..; \
	cd samples; $(MAKE) clean; cd ..

TOPDIR=.
include Makerules

ifeq ($(USEGLP), yes)
test: rr mesademos
	chmod u+x mesademos/dotests ;\
	exec mesademos/dotests $(EDIR) $(platform) $(subplatform) GLP
else
test: rr mesademos
	chmod u+x mesademos/dotests ;\
	exec mesademos/dotests $(EDIR) $(platform) $(subplatform)
endif

##########################################################################
ifeq ($(platform), windows)
##########################################################################

ifneq ($(DISTRO),)
WBLDDIR = $(platform)$(subplatform)\\$(DISTRO)
else
WBLDDIR = $(platform)$(subplatform)
endif

ifeq ($(DEBUG), yes)
WBLDDIR := $(WBLDDIR)\\dbg
endif

dist: rr diags
	$(RM) $(WBLDDIR)\$(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DBLDDIR=$(WBLDDIR) rr.nsi || \
	makensis /DAPPNAME=$(APPNAME) /DVERSION=$(VERSION) \
		/DBUILD=$(BUILD) /DBLDDIR=$(WBLDDIR) rr.nsi  # Cygwin doesn't like the //


##########################################################################
else
##########################################################################

ifeq ($(subplatform),)
RPMARCH = i386
else
RPMARCH = $(ARCH)
endif

ifeq ($(prefix),)
prefix=/usr/local
endif

lib64dir=lib64
ifeq ($(platform), solaris)
lib64dir=lib/sparcv9
endif
ifeq ($(platform), solx86)
lib64dir=lib/64
endif

PACKAGENAME = $(APPNAME)
ifeq ($(subplatform), 64)
PACKAGENAME = $(APPNAME)64
endif

ifeq ($(subplatform), 64)
install: rr
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/$(lib64dir) ]; then mkdir -p $(prefix)/$(lib64dir); fi
	if [ ! -d $(prefix)/doc/$(PACKAGENAME)/samples ]; then mkdir -p $(prefix)/doc/$(PACKAGENAME)/samples; fi
	if [ ! -d $(prefix)/include ]; then mkdir -p $(prefix)/include; fi
	$(INSTALL) -m 755 $(EDIR)/vglrun64 $(prefix)/bin/vglrun64
	$(INSTALL) -m 755 $(EDIR)/vglrun64 $(prefix)/bin/rrlaunch64
	$(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librr.$(SHEXT) $(prefix)/$(lib64dir)/librr.$(SHEXT)
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 LICENSE-OpenSSL.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 rr/rr.h $(prefix)/include
	$(INSTALL) -m 644 samples/rrglxgears.c $(prefix)/doc/$(PACKAGENAME)/samples/rrglxgears.c
	$(INSTALL) -m 644 samples/Makefile.$(platform)$(subplatform) $(prefix)/doc/$(PACKAGENAME)/samples/Makefile
	echo Install complete.
else
install: rr diags
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/lib ]; then mkdir -p $(prefix)/lib; fi
	if [ ! -d $(prefix)/doc/$(PACKAGENAME)/samples ]; then mkdir -p $(prefix)/doc/$(PACKAGENAME)/samples; fi
	if [ ! -d $(prefix)/include ]; then mkdir -p $(prefix)/include; fi
	$(INSTALL) -m 755 rr/rrxclient.sh $(prefix)/bin/vglclient_daemon
	$(INSTALL) -m 755 rr/rrxclient_ssl.sh $(prefix)/bin/vglclient_ssldaemon
	$(INSTALL) -m 755 rr/rrxclient_config $(prefix)/bin/vglclient_config
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/rrlaunch
	$(INSTALL) -m 755 $(EDIR)/vglclient $(prefix)/bin/vglclient
	$(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/lib/libturbojpeg.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/lib/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librr.$(SHEXT) $(prefix)/lib/librr.$(SHEXT)
	$(INSTALL) -m 755 $(EDIR)/tcbench $(prefix)/bin/tcbench
	$(INSTALL) -m 755 $(EDIR)/nettest $(prefix)/bin/nettest
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 LICENSE-OpenSSL.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc/$(PACKAGENAME)
	$(INSTALL) -m 644 rr/rr.h $(prefix)/include
	$(INSTALL) -m 644 samples/rrglxgears.c $(prefix)/doc/$(PACKAGENAME)/samples/rrglxgears.c
	$(INSTALL) -m 644 samples/Makefile.$(platform)$(subplatform) $(prefix)/doc/$(PACKAGENAME)/samples/Makefile
	echo Install complete.
endif

ifeq ($(subplatform), 64)
uninstall:
	$(RM) $(prefix)/bin/vglrun64
	$(RM) $(prefix)/bin/rrlaunch64
	$(RM) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/librr.$(SHEXT)
	$(RM) $(prefix)/doc/$(PACKAGENAME)/samples/*
	rmdir $(prefix)/doc/$(PACKAGENAME)/samples
	$(RM) $(prefix)/doc/$(PACKAGENAME)/*
	rmdir $(prefix)/doc/$(PACKAGENAME)
	$(RM) $(prefix)/include/rr.h
	echo Uninstall complete.
else
uninstall:
	$(prefix)/bin/vglclient_daemon stop || echo Client not installed as a service
	$(prefix)/bin/vglclient_ssldaemon stop || echo Secure client not installed as a service
	$(RM) $(prefix)/bin/vglclient_daemon
	$(RM) $(prefix)/bin/vglclient_ssldaemon
	$(RM) $(prefix)/bin/vglclient_config
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/bin/vglclient
	$(RM) $(prefix)/lib/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/lib/librrfaker.$(SHEXT)
	$(RM) $(prefix)/lib/librr.$(SHEXT)
	$(RM) $(prefix)/bin/tcbench
	$(RM) $(prefix)/bin/nettest
	$(RM) $(prefix)/doc/$(PACKAGENAME)/samples/*
	rmdir $(prefix)/doc/$(PACKAGENAME)/samples
	$(RM) $(prefix)/doc/$(PACKAGENAME)/*
	rmdir $(prefix)/doc/$(PACKAGENAME)
	$(RM) $(prefix)/include/rr.h
	echo Uninstall complete.
endif

ifeq ($(platform), linux)

dist: rr diags
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	ln -fs `pwd` $(BLDDIR)/rpms/BUILD
	rm -f $(BLDDIR)/$(PACKAGENAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir `pwd`/$(BLDDIR)" --define "_topdir $(BLDDIR)/rpms" \
		--define "_version $(VERSION)" --define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_libdir $(LDIR)" --define "_appname $(APPNAME)" --target $(RPMARCH) \
		rr.spec; \
	mv $(BLDDIR)/rpms/RPMS/$(RPMARCH)/$(PACKAGENAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(BLDDIR)/$(PACKAGENAME).$(RPMARCH).rpm
	rm -rf $(BLDDIR)/rpms

endif

ifeq ($(platform), solaris)
dist: sunpkg
endif
ifeq ($(platform), solx86)
dist: sunpkg
endif

PKGDIR = SUNWvgl
PKGNAME = $(PKGDIR)
PKGARCH = sparc
ifeq ($(platform), solx86)
PKGARCH = i386
endif

.PHONY: sunpkg
sunpkg: rr diags
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(PKGNAME)
	rm -f $(BLDDIR)/$(PKGNAME).pkg.bz2
	mkdir -p $(BLDDIR)/pkgbuild/$(PKGDIR)
	mkdir -p $(BLDDIR)/$(PKGNAME)
	cp copyright $(BLDDIR)
	cp depend.$(platform) $(BLDDIR)/depend
	cp rr.proto.$(platform)$(subplatform) $(BLDDIR)/rr.proto
	cat pkginfo.tmpl | sed s/{__VERSION}/$(VERSION)/g | sed s/{__BUILD}/$(BUILD)/g \
		| sed s/{__APPNAME}/$(APPNAME)/g | sed s/{__PKGNAME}/$(PKGNAME)/g >$(BLDDIR)/pkginfo \
		| sed s/{__PKGARCH}/$(PKGARCH)/g
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(PKGDIR) install
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(PKGDIR) M32=yes install
	cd $(BLDDIR); \
	pkgmk -o -r ./pkgbuild -d . -a `uname -p` -f rr.proto; \
	rm rr.proto copyright depend pkginfo; \
	pkgtrans -s `pwd` `pwd`/$(PKGNAME).pkg $(PKGNAME)
	bzip2 $(BLDDIR)/$(PKGNAME).pkg
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(PKGNAME)

.PHONY: tarball
tarball: rr diags
	rm -rf $(BLDDIR)/fakeroot
	rm -f $(BLDDIR)/$(PACKAGENAME).tar.gz
	$(MAKE) prefix=$(BLDDIR)/fakeroot install
	cd $(BLDDIR)/fakeroot; \
	tar cf ../$(PACKAGENAME).tar *
	gzip $(BLDDIR)/$(PACKAGENAME).tar
	rm -rf $(BLDDIR)/fakeroot

##########################################################################
endif
##########################################################################
