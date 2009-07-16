all: rr mesademos diags
ALL: dist mesademos

.PHONY: rr util jpeg mesademos diags clean dist test fltk x11windows putty

vnc:
	$(MAKE) DISTRO=vnc VNC=yes jpeg

rr: util jpeg fltk x11windows putty
jpeg: util

rr util jpeg mesademos diags fltk x11windows putty:
	cd $@; $(MAKE); cd ..

clean:
	cd rr; $(MAKE) clean; cd ..; \
	cd util; $(MAKE) clean; cd ..; \
	cd jpeg; $(MAKE) clean; cd ..; \
	cd mesademos; $(MAKE) clean; cd ..; \
	cd diags; $(MAKE) clean; cd ..; \
	cd fltk; $(MAKE) clean; cd ..; \
	cd x11windows; $(MAKE) clean; cd ..; \
	cd putty; $(MAKE) clean; cd ..;

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
 JPGDIR=$$%systemroot%\\system32
 JPGLIB=turbojpeg.dll
else
 ifeq ($(JPEGLIB), medialib)
  JPGDIR=..\\openmliblite\\$(WBLDDIR)\\bin
  JPGLIB=openmliblite.dll
 else
  JPGDIR=$(WBLDDIR)\\bin
  JPGLIB=turbojpeg.dll
 endif
endif

dist: rr diags
	$(RM) $(WBLDDIR)\$(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DBLDDIR=$(WBLDDIR) //DJPGDIR=$(JPGDIR) \
		//DJPGLIB=$(JPGLIB) rr.nsi || \
	makensis /DAPPNAME=$(APPNAME) /DVERSION=$(VERSION) \
		/DBUILD=$(BUILD) /DBLDDIR=$(WBLDDIR) /DJPGDIR=$(JPGDIR) \
		/DJPGLIB=$(JPGLIB) rr.nsi  # Cygwin doesn't like the //


##########################################################################
else
##########################################################################

ifeq ($(prefix),)
 prefix=/usr/local
endif

lib64dir=lib
ifeq ($(subplatform), 64)
 lib64dir=lib64
 ifeq ($(platform), solaris)
  lib64dir=lib/sparcv9
 endif
 ifeq ($(platform), solx86)
  lib64dir=lib/amd64
 endif
endif

ifeq ($(subplatform), 64)
install: rr
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/$(lib64dir) ]; then mkdir -p $(prefix)/$(lib64dir); fi
	if [ ! -d $(prefix)/doc ]; then mkdir -p $(prefix)/doc; fi
	$(INSTALL) -m 755 $(EDIR)/vgllogin $(prefix)/bin/vgllogin
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 rr/vglgenkey $(prefix)/bin/vglgenkey
	$(INSTALL) -m 755 rr/vglserver_config $(prefix)/bin/vglserver_config
	if [ "$(JPEGLIB)" = "medialib" ]; then $(INSTALL) -m 755 $(EDIR)/vglclient $(prefix)/bin/vglclient; fi
	if [ -f $(LDIR)/libturbojpeg.$(SHEXT) ]; then $(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT); fi
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libdlfaker.$(SHEXT) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libgefaker.$(SHEXT) $(prefix)/$(lib64dir)/libgefaker.$(SHEXT)
	$(INSTALL) -m 755 $(EDIR)/glxspheres $(prefix)/bin/glxspheres64
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc
	$(INSTALL) -m 644 fltk/COPYING $(prefix)/doc/LICENSE-FLTK.txt
	$(INSTALL) -m 644 putty/LICENCE $(prefix)/doc/LICENSE-PuTTY.txt
	$(INSTALL) -m 644 x11windows/xauth.license $(prefix)/doc/LICENSE-xauth.txt
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc
	$(INSTALL) -m 644 ChangeLog.txt $(prefix)/doc
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc
	$(INSTALL) -m 644 doc/*.css $(prefix)/doc
	echo Install complete.
else
install: rr diags mesademos
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/lib ]; then mkdir -p $(prefix)/lib; fi
	if [ ! -d $(prefix)/doc ]; then mkdir -p $(prefix)/doc; fi
	$(INSTALL) -m 755 $(EDIR)/vgllogin $(prefix)/bin/vgllogin
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 rr/vglgenkey $(prefix)/bin/vglgenkey
	$(INSTALL) -m 755 rr/vglserver_config $(prefix)/bin/vglserver_config
	$(INSTALL) -m 755 $(EDIR)/vglclient $(prefix)/bin/vglclient
	$(INSTALL) -m 755 $(EDIR)/vglconfig $(prefix)/bin/vglconfig
	$(INSTALL) -m 755 $(EDIR)/vglconnect $(prefix)/bin/vglconnect
	if [ -f $(LDIR)/libturbojpeg.$(SHEXT) ]; then $(INSTALL) -m 755 $(LDIR)/libturbojpeg.$(SHEXT) $(prefix)/lib/libturbojpeg.$(SHEXT); fi
	$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/lib/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libdlfaker.$(SHEXT) $(prefix)/lib/libdlfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR)/libgefaker.$(SHEXT) $(prefix)/lib/libgefaker.$(SHEXT)
	$(INSTALL) -m 755 $(EDIR)/tcbench $(prefix)/bin/tcbench
	$(INSTALL) -m 755 $(EDIR)/nettest $(prefix)/bin/nettest
	$(INSTALL) -m 755 $(EDIR)/glxinfo $(prefix)/bin/glxinfo
	$(INSTALL) -m 755 $(EDIR)/glxspheres $(prefix)/bin/glxspheres
	$(INSTALL) -m 644 LGPL.txt $(prefix)/doc
	$(INSTALL) -m 644 fltk/COPYING $(prefix)/doc/LICENSE-FLTK.txt
	$(INSTALL) -m 644 putty/LICENCE $(prefix)/doc/LICENSE-PuTTY.txt
	$(INSTALL) -m 644 x11windows/xauth.license $(prefix)/doc/LICENSE-xauth.txt
	$(INSTALL) -m 644 LICENSE.txt $(prefix)/doc
	$(INSTALL) -m 644 ChangeLog.txt $(prefix)/doc
	$(INSTALL) -m 644 doc/index.html $(prefix)/doc
	$(INSTALL) -m 644 doc/*.gif $(prefix)/doc
	$(INSTALL) -m 644 doc/*.png $(prefix)/doc
	$(INSTALL) -m 644 doc/*.css $(prefix)/doc
	echo Install complete.
endif

ifeq ($(subplatform), 64)
uninstall:
	$(RM) $(prefix)/bin/vgllogin
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/vglserver_config
	$(RM) $(prefix)/$(lib64dir)/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT)
	$(RM) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT)
	$(RM) $(prefix)/doc/*
	if [ -d $(prefix)/doc ]; then rmdir $(prefix)/doc; fi
	echo Uninstall complete.
else
uninstall:
	$(RM) $(prefix)/bin/vgllogin
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/vglserver_config
	$(RM) $(prefix)/bin/vglclient
	$(RM) $(prefix)/bin/vglconfig
	$(RM) $(prefix)/bin/vglconnect
	$(RM) $(prefix)/lib/libturbojpeg.$(SHEXT)
	$(RM) $(prefix)/lib/librrfaker.$(SHEXT)
	$(RM) $(prefix)/lib/libdlfaker.$(SHEXT)
	$(RM) $(prefix)/bin/tcbench
	$(RM) $(prefix)/bin/nettest
	$(RM) $(prefix)/doc/*
	if [ -d $(prefix)/doc ]; then rmdir $(prefix)/doc; fi
	echo Uninstall complete.
endif

ifeq ($(platform), linux)

.PHONY: rr32 diags32 mesademos32
ifeq ($(subplatform),)
rr32: rr
diags32: diags
mesademos32: mesademos
else
rr32:
	$(MAKE) M32=yes rr
diags32:
	$(MAKE) M32=yes diags
mesademos32:
	$(MAKE) M32=yes mesademos
endif

dist: rr rr32 diags32 mesademos mesademos32
	TMPDIR=`mktemp -d /tmp/vglbuild.XXXXXX`; \
	mkdir -p $$TMPDIR/RPMS; \
	ln -fs `pwd` $$TMPDIR/BUILD; \
	rm -f $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir $$TMPDIR/buildroot" \
		--define "_topdir $$TMPDIR" --define "_version $(VERSION)"\
		--define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_bindir32 $(EDIR32)" --define "_libdir $(LDIR)" \
		--define "_libdir32 $(LDIR32)" --define "_omlibdir $(OMLDIR)" \
		--define "_omlibdir32 $(OMLDIR32)" --target $(RPMARCH) \
		rr.spec.$(JPEGLIB); \
	cp $$TMPDIR/RPMS/$(RPMARCH)/$(APPNAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm; \
	rm -rf $$TMPDIR

deb: rr rr32 diags32 mesademos mesademos32
	sh makedpkg $(APPNAME) $(JPEGLIB) $(BLDDIR)/$(APPNAME)_$(DEBARCH).deb $(VERSION) $(BUILD) $(DEBARCH) $(LDIR) $(LDIR32) $(EDIR) $(EDIR32) $(OMLDIR) $(OMLDIR32)

srpm:
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	mkdir -p $(BLDDIR)/rpms/SRPMS
	mkdir -p $(BLDDIR)/rpms/BUILD
	mkdir -p $(BLDDIR)/rpms/SOURCES
	cp vgl.tar.gz $(BLDDIR)/rpms/SOURCES/$(APPNAME)-$(VERSION).tar.gz
	cp openmliblite.tar.gz $(BLDDIR)/rpms/SOURCES/openmliblite-vgl$(VERSION).tar.gz
	cat rr.spec.$(JPEGLIB) | sed s/%{_version}/$(VERSION)/g \
		| sed s/%{_build}/$(BUILD)/g | sed s/%{_blddir}/%{_tmppath}/g \
		| sed s/%{_bindir32}/linux\\/bin/g | sed s/%{_bindir}/linux64\\/bin/g \
		| sed s/%{_libdir32}/linux\\/lib/g | sed s/%{_libdir}/linux64\\/lib/g \
		| sed s/%{_omlibdir32}/..\\/openmliblite\\/linux\\/lib/g \
		| sed s/%{_omlibdir}/..\\/openmliblite\\/linux64\\/lib/g \
		| sed s/#--\>//g >$(BLDDIR)/virtualgl.spec
	rpmbuild -bs --define "_topdir `pwd`/$(BLDDIR)/rpms" --target $(RPMARCH) $(BLDDIR)/virtualgl.spec
	mv $(BLDDIR)/rpms/SRPMS/$(APPNAME)-$(VERSION)-$(BUILD).src.rpm $(BLDDIR)/$(APPNAME).src.rpm
	rm -rf $(BLDDIR)/rpms

endif

ifeq ($(platform), cygwin)
dist: rr diags mesademos
	sh makecygwinpkg $(APPNAME) $(JPEGLIB) $(BLDDIR) $(VERSION) $(BUILD) $(EDIR) $(OMEDIR)
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
sunpkg: rr diags mesademos
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(PKGNAME)
	rm -f $(BLDDIR)/$(PKGNAME).pkg.bz2
	mkdir -p $(BLDDIR)/pkgbuild/$(PKGDIR)
	mkdir -p $(BLDDIR)/$(PKGNAME)
	cp copyright $(BLDDIR)
	cp depend.$(platform) $(BLDDIR)/depend
	cat pkginfo.tmpl | sed s/{__VERSION}/$(VERSION)/g | sed s/{__BUILD}/$(BUILD)/g \
		| sed s/{__APPNAME}/$(APPNAME)/g | sed s/{__PKGNAME}/$(PKGNAME)/g \
		| sed s/{__PKGARCH}/$(PKGARCH)/g >$(BLDDIR)/pkginfo
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(PKGDIR) M32=yes install
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(PKGDIR) install
	sh makeproto $(BLDDIR)/pkgbuild/$(PKGDIR) $(platform) $(subplatform) > $(BLDDIR)/rr.proto
	cd $(BLDDIR); \
	pkgmk -o -r ./pkgbuild -d . -a `uname -p` -f rr.proto; \
	rm rr.proto copyright depend pkginfo; \
	pkgtrans -s `pwd` `pwd`/$(PKGNAME).pkg $(PKGNAME)
	bzip2 $(BLDDIR)/$(PKGNAME).pkg
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(PKGNAME)

ifeq ($(platform), osxx86)
dist: rr diags mesademos
	sh makemacpkg $(APPNAME) $(JPEGLIB) $(BLDDIR) $(VERSION) $(BUILD) $(EDIR) $(OMLDIR)
endif

.PHONY: tarball
tarball: rr diags mesademos
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
