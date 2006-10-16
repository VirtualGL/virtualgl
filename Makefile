all: rr mesademos diags samples
ALL: dist mesademos

.PHONY: rr util jpeg mesademos diags clean dist samples test

vnc:
	$(MAKE) DISTRO=vnc VNC=yes jpeg

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

ifeq ($(JPEGLIB), ipp)
TJDIR=$$%systemroot%\\system32
else
TJDIR=$(WBLDDIR)\\bin
endif

dist: rr diags
	$(RM) $(WBLDDIR)\$(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DBLDDIR=$(WBLDDIR) //DTJDIR=$(TJDIR) rr.nsi || \
	makensis /DAPPNAME=$(APPNAME) /DVERSION=$(VERSION) \
		/DBUILD=$(BUILD) /DBLDDIR=$(WBLDDIR) /DTJDIR=$(TJDIR) rr.nsi  # Cygwin doesn't like the //


##########################################################################
else
##########################################################################

ifeq ($(prefix),)
prefix=/usr/local
endif

lib64dir=lib64
ifeq ($(platform), solaris)
lib64dir=lib/sparcv9
endif
ifeq ($(platform), solx86)
lib64dir=lib/amd64
endif

ifeq ($(subplatform), 64)
install: rr
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/$(lib64dir) ]; then mkdir -p $(prefix)/$(lib64dir); fi
	if [ ! -d $(prefix)/doc/samples ]; then mkdir -p $(prefix)/doc/samples; fi
	if [ ! -d $(prefix)/include ]; then mkdir -p $(prefix)/include; fi
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/rrlaunch
	if [ -f $(LDIR)/libturbojpeg.$(SHEXT) ]; then $(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT); fi
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libdlfaker.$(SHEXT) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librr.$(SHEXT) $(prefix)/$(lib64dir)/librr.$(SHEXT)
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc
	$(INSTALL) -m 644 LICENSE-OpenSSL.txt $(prefix)/doc
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc
	$(INSTALL) -m 644 rr/rr.h $(prefix)/include
	$(INSTALL) -m 644 samples/rrglxgears.c $(prefix)/doc/samples
	$(INSTALL) -m 644 samples/Makefile.$(platform)$(subplatform) $(prefix)/doc/samples
	echo Install complete.
else
install: rr diags
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/lib ]; then mkdir -p $(prefix)/lib; fi
	if [ ! -d $(prefix)/doc/samples ]; then mkdir -p $(prefix)/doc/samples; fi
	if [ ! -d $(prefix)/include ]; then mkdir -p $(prefix)/include; fi
	$(INSTALL) -m 755 rr/rrxclient.sh $(prefix)/bin/vglclient_daemon
	$(INSTALL) -m 755 rr/rrxclient_config $(prefix)/bin/vglclient_config
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/rrlaunch
	$(INSTALL) -m 755 $(EDIR)/vglclient $(prefix)/bin/vglclient
	if [ -f $(LDIR)/libturbojpeg.$(SHEXT) ]; then $(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/lib/libturbojpeg.$(SHEXT); fi
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/lib/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libdlfaker.$(SHEXT) $(prefix)/lib/libdlfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/librr.$(SHEXT) $(prefix)/lib/librr.$(SHEXT)
	$(INSTALL) -m 755 $(EDIR)/tcbench $(prefix)/bin/tcbench
	$(INSTALL) -m 755 $(EDIR)/nettest $(prefix)/bin/nettest
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc
	$(INSTALL) -m 644 LICENSE-OpenSSL.txt $(prefix)/doc
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc
	$(INSTALL) -m 644 rr/rr.h $(prefix)/include
	$(INSTALL) -m 644 samples/rrglxgears.c $(prefix)/doc/samples
	$(INSTALL) -m 644 samples/Makefile.$(platform)$(subplatform) $(prefix)/doc/samples
	echo Install complete.
endif

ifeq ($(subplatform), 64)
uninstall:
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/librr.$(SHEXT)
	$(RM) $(prefix)/doc/samples/*
	if [ -d $(prefix)/doc/samples ]; then rmdir $(prefix)/doc/samples; fi
	$(RM) $(prefix)/doc/*
	if [ -d $(prefix)/doc ]; then rmdir $(prefix)/doc; fi
	$(RM) $(prefix)/include/rr.h
	echo Uninstall complete.
else
uninstall:
	$(prefix)/bin/vglclient_daemon stop || echo Client not installed as a service
	$(RM) $(prefix)/bin/vglclient_daemon
	$(RM) $(prefix)/bin/vglclient_config
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/bin/vglclient
	$(RM) $(prefix)/lib/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/lib/librrfaker.$(SHEXT)
	$(RM) $(prefix)/lib/libdlfaker.$(SHEXT)
	$(RM) $(prefix)/lib/librr.$(SHEXT)
	$(RM) $(prefix)/bin/tcbench
	$(RM) $(prefix)/bin/nettest
	$(RM) $(prefix)/doc/samples/*
	if [ -d $(prefix)/doc/samples ]; then rmdir $(prefix)/doc/samples; fi
	$(RM) $(prefix)/doc/*
	if [ -d $(prefix)/doc ]; then rmdir $(prefix)/doc; fi
	$(RM) $(prefix)/include/rr.h
	echo Uninstall complete.
endif

ifeq ($(platform), linux)

.PHONY: rr32 diags32
ifeq ($(subplatform),)
rr32: rr
diags32: diags
else
rr32:
	$(MAKE) M32=yes rr
diags32:
	$(MAKE) M32=yes diags
endif

dist: rr rr32 diags32
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	ln -fs `pwd` $(BLDDIR)/rpms/BUILD
	rm -f $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir `pwd`/$(BLDDIR)" --define "_topdir $(BLDDIR)/rpms" \
		--define "_version $(VERSION)" --define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_bindir32 $(EDIR32)" --define "_libdir $(LDIR)" --define "_libdir32 $(LDIR32)" \
		--target $(RPMARCH) \
		rr.spec; \
	mv $(BLDDIR)/rpms/RPMS/$(RPMARCH)/$(APPNAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm
	rm -rf $(BLDDIR)/rpms

srpm:
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	mkdir -p $(BLDDIR)/rpms/SRPMS
	mkdir -p $(BLDDIR)/rpms/BUILD
	mkdir -p $(BLDDIR)/rpms/SOURCES
	cp vgl.tar.gz $(BLDDIR)/rpms/SOURCES/$(APPNAME)-$(VERSION).tar.gz
	cat rr.spec | sed s/%{_version}/$(VERSION)/g | sed s/%{_build}/$(BUILD)/g \
		| sed s/%{_blddir}/%{_tmppath}/g | sed s/%{_bindir32}/linux\\/bin/g \
		| sed s/%{_bindir}/linux\\/bin/g | sed s/%{_libdir32}/linux\\/lib/g \
		| sed s/%{_libdir}/linux64\\/lib/g | sed s/#--\>//g >$(BLDDIR)/virtualgl.spec
	rpmbuild -ba --define "_topdir `pwd`/$(BLDDIR)/rpms" --target $(RPMARCH) $(BLDDIR)/virtualgl.spec
	mv $(BLDDIR)/rpms/RPMS/$(RPMARCH)/$(APPNAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm
	mv $(BLDDIR)/rpms/SRPMS/$(APPNAME)-$(VERSION)-$(BUILD).src.rpm $(BLDDIR)/$(APPNAME).src.rpm
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
	rm -f $(BLDDIR)/$(APPNAME).tar.gz
	$(MAKE) prefix=$(BLDDIR)/fakeroot install
	cd $(BLDDIR)/fakeroot; \
	tar cf ../$(APPNAME).tar *
	gzip $(BLDDIR)/$(APPNAME).tar
	rm -rf $(BLDDIR)/fakeroot

##########################################################################
endif
##########################################################################
