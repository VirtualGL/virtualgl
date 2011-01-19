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

ifeq ($(subplatform), 64)
 WIN64DEF1 := //DWIN64
 WIN64DEF2 := /DWIN64
endif

dist: rr diags
	$(RM) $(WBLDDIR)/$(APPNAME)$(subplatform).exe
	$(RM) $(WBLDDIR)/$(APPNAME)$(subplatform)-exceed.exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) $(WIN64DEF1) \
		//DBUILD=$(BUILD) //DBLDDIR=$(WBLDDIR) rr.nsi || \
	makensis /DAPPNAME=$(APPNAME) /DVERSION=$(VERSION) $(WIN64DEF2) \
		/DBUILD=$(BUILD) /DBLDDIR=$(WBLDDIR) rr.nsi  # Cygwin doesn't like the //
	mv $(WBLDDIR)/$(APPNAME)$(subplatform).exe $(WBLDDIR)/$(APPNAME)$(subplatform)-exceed.exe

##########################################################################
else
##########################################################################

ifeq ($(prefix),)
 prefix=/opt/VirtualGL
endif

.PHONY: rr32 mesademos32 diags32
ifeq ($(subplatform),)
rr32: rr
mesademos32: mesademos
diags32: diags
else
rr32:
	$(MAKE) M32=yes rr
mesademos32:
	$(MAKE) M32=yes mesademos
diags32:
	$(MAKE) M32=yes diags
endif

libdir=$(shell if [ -d /usr/lib32 ]; then echo lib32; else echo lib; fi)
ifeq ($(subplatform), 64)
 lib64dir=$(shell if [ -d /usr/lib32 ]; then echo lib; else echo lib64; fi)
 ifeq ($(platform), solaris)
  lib64dir=lib/sparcv9
 endif
 ifeq ($(platform), solx86)
  lib64dir=lib/amd64
 endif
endif

docdir=$(prefix)/doc/$(APPNAME)-$(VERSION)

install: rr rr32 diags mesademos mesademos32
	if [ ! -d $(prefix)/bin ]; then mkdir -p $(prefix)/bin; fi
	if [ ! -d $(prefix)/include ]; then mkdir -p $(prefix)/include; fi
	if [ ! -d $(prefix)/fakelib ]; then mkdir -p $(prefix)/fakelib; fi
	if [ "$(subplatform)" = "64" ]; then \
		if [ ! -d $(prefix)/$(lib64dir) ]; then mkdir -p $(prefix)/$(lib64dir); fi; \
		if [ ! -d $(prefix)/fakelib/64 ]; then mkdir -p $(prefix)/fakelib/64; fi; \
	fi
	if [ ! -d $(prefix)/$(libdir) ]; then mkdir -p $(prefix)/$(libdir); fi
	if [ ! -d $(docdir) ]; then mkdir -p $(docdir); fi
	$(INSTALL) -m 755 $(EDIR)/vglclient $(prefix)/bin/vglclient
	$(INSTALL) -m 755 $(EDIR)/vglconfig $(prefix)/bin/vglconfig
	$(INSTALL) -m 755 $(EDIR)/vglconnect $(prefix)/bin/vglconnect
	$(INSTALL) -m 755 $(EDIR)/vgllogin $(prefix)/bin/vgllogin
	$(INSTALL) -m 755 $(EDIR)/vglrun $(prefix)/bin/vglrun
	$(INSTALL) -m 755 rr/vglgenkey $(prefix)/bin/vglgenkey
	$(INSTALL) -m 755 rr/vglserver_config $(prefix)/bin/vglserver_config
	if [ "$(platform)" = "linux" ]; then \
		$(INSTALL) -m 755 $(EDIR)/cpustat $(prefix)/bin/cpustat; \
	fi
	$(INSTALL) -m 755 $(EDIR)/tcbench $(prefix)/bin/tcbench
	$(INSTALL) -m 755 $(EDIR)/nettest $(prefix)/bin/nettest
	$(INSTALL) -m 755 $(EDIR)/glxinfo $(prefix)/bin/glxinfo
	$(INSTALL) -m 755 $(EDIR32)/glxspheres $(prefix)/bin/glxspheres
	$(INSTALL) -m 755 $(LDIR32)/librrfaker.$(SHEXT) $(prefix)/$(libdir)/librrfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR32)/libdlfaker.$(SHEXT) $(prefix)/$(libdir)/libdlfaker.$(SHEXT)
	$(INSTALL) -m 755 $(LDIR32)/libgefaker.$(SHEXT) $(prefix)/$(libdir)/libgefaker.$(SHEXT)
	ln -fs ../$(libdir)/librrfaker.so $(prefix)/fakelib/libGL.so
	if [ "$(subplatform)" = "64" ]; then \
		$(INSTALL) -m 755 $(LDIR)/librrfaker.$(SHEXT) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT); \
		$(INSTALL) -m 755 $(LDIR)/libdlfaker.$(SHEXT) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT); \
		$(INSTALL) -m 755 $(LDIR)/libgefaker.$(SHEXT) $(prefix)/$(lib64dir)/libgefaker.$(SHEXT); \
		$(INSTALL) -m 755 $(EDIR)/glxspheres $(prefix)/bin/glxspheres64; \
		ln -fs ../../$(lib64dir)/librrfaker.so $(prefix)/fakelib/64/libGL.so; \
	fi
	$(INSTALL) -m 644 LGPL.txt $(docdir)
	$(INSTALL) -m 644 fltk/COPYING $(docdir)/LICENSE-FLTK.txt
	$(INSTALL) -m 644 putty/LICENCE $(docdir)/LICENSE-PuTTY.txt
	$(INSTALL) -m 644 x11windows/xauth.license $(docdir)/LICENSE-xauth.txt
	$(INSTALL) -m 644 LICENSE.txt $(docdir)
	$(INSTALL) -m 644 ChangeLog.txt $(docdir)
	$(INSTALL) -m 644 doc/index.html $(docdir)
	$(INSTALL) -m 644 doc/*.gif $(docdir)
	$(INSTALL) -m 644 doc/*.png $(docdir)
	$(INSTALL) -m 644 doc/*.css $(docdir)
	$(INSTALL) -m 644 rr/rrtransport.h $(prefix)/include
	$(INSTALL) -m 644 rr/rr.h $(prefix)/include
	echo Install complete.

uninstall:
	$(RM) $(prefix)/bin/vglclient
	$(RM) $(prefix)/bin/vglconfig
	$(RM) $(prefix)/bin/vglconnect
	$(RM) $(prefix)/bin/vgllogin
	$(RM) $(prefix)/bin/vglrun
	$(RM) $(prefix)/bin/vglgenkey
	$(RM) $(prefix)/bin/vglserver_config
	$(RM) $(prefix)/bin/tcbench
	if [ "$(platform)" = "linux" ]; then \
		$(RM) $(prefix)/bin/cpustat; \
	fi
	$(RM) $(prefix)/bin/nettest
	$(RM) $(prefix)/bin/glxinfo
	$(RM) $(prefix)/bin/glxspheres
	$(RM) $(prefix)/$(libdir)/librrfaker.$(SHEXT)
	$(RM) $(prefix)/$(libdir)/libdlfaker.$(SHEXT)
	$(RM) $(prefix)/$(libdir)/libgefaker.$(SHEXT)
	if [ "$(subplatform)" = "64" ]; then \
		$(RM) $(prefix)/$(lib64dir)/librrfaker.$(SHEXT); \
		$(RM) $(prefix)/$(lib64dir)/libdlfaker.$(SHEXT); \
		$(RM) $(prefix)/$(lib64dir)/libgefaker.$(SHEXT); \
		$(RM) $(prefix)/bin/glxspheres64; \
		if [ -d $(prefix)/fakelib/64 ]; then \
			$(RM) $(prefix)/fakelib/64/*;  rmdir $(prefix)/fakelib/64; \
		fi; \
	fi
	if [ -d $(prefix)/fakelib ]; then \
		$(RM) $(prefix)/fakelib/*;  rmdir $(prefix)/fakelib; \
	fi
	$(RM) $(docdir)/*
	if [ -d $(docdir) ]; then rmdir $(docdir); fi
	$(RM) $(prefix)/include/rrtransport.h
	$(RM) $(prefix)/include/rr.h
	echo Uninstall complete.

ifeq ($(platform), linux)

dist: rr rr32 diags mesademos mesademos32
	TMPDIR=`mktemp -d /tmp/vglbuild.XXXXXX`; \
	mkdir -p $$TMPDIR/RPMS; \
	ln -fs `pwd` $$TMPDIR/BUILD; \
	rm -f $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir $$TMPDIR/buildroot" \
		--define "_topdir $$TMPDIR" --define "_version $(VERSION)"\
		--define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_bindir32 $(EDIR32)" --define "_libdir $(LDIR)" \
		--define "_libdir32 $(LDIR32)" --target $(RPMARCH) \
		rr.spec; \
	cp $$TMPDIR/RPMS/$(RPMARCH)/$(APPNAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(BLDDIR)/$(APPNAME).$(RPMARCH).rpm; \
	rm -rf $$TMPDIR

deb: rr rr32 diags mesademos mesademos32
	sh makedpkg $(APPNAME) $(BLDDIR)/$(APPNAME)_$(DEBARCH).deb $(VERSION) $(BUILD) $(DEBARCH) $(LDIR) $(LDIR32) $(EDIR) $(EDIR32)

srpm:
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	mkdir -p $(BLDDIR)/rpms/SRPMS
	mkdir -p $(BLDDIR)/rpms/BUILD
	mkdir -p $(BLDDIR)/rpms/SOURCES
	cp vgl.tar.gz $(BLDDIR)/rpms/SOURCES/$(APPNAME)-$(VERSION).tar.gz
	cat rr.spec | sed s/%{_version}/$(VERSION)/g \
		| sed s/%{_build}/$(BUILD)/g | sed s/%{_blddir}/%{_tmppath}/g \
		| sed s/%{_bindir32}/linux\\/bin/g | sed s/%{_bindir}/linux64\\/bin/g \
		| sed s/%{_libdir32}/linux\\/lib/g | sed s/%{_libdir}/linux64\\/lib/g \
		| sed s/#--\>//g >$(BLDDIR)/virtualgl.spec
	rpmbuild -bs --define "_topdir `pwd`/$(BLDDIR)/rpms" --target $(RPMARCH) $(BLDDIR)/virtualgl.spec
	mv $(BLDDIR)/rpms/SRPMS/$(APPNAME)-$(VERSION)-$(BUILD).src.rpm $(BLDDIR)/$(APPNAME).src.rpm
	rm -rf $(BLDDIR)/rpms

endif

ifeq ($(platform), cygwin)
dist: rr diags mesademos
	sh makecygwinpkg $(APPNAME) $(BLDDIR) $(VERSION) $(EDIR)
endif

ifeq ($(platform), solaris)
dist: sunpkg
endif
ifeq ($(platform), solx86)
dist: sunpkg
endif

PKGARCH = sparc
ifeq ($(platform), solx86)
PKGARCH = i386
endif

.PHONY: sunpkg
sunpkg: rr diags mesademos
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(APPNAME)
	rm -f $(BLDDIR)/$(APPNAME).pkg.bz2
	mkdir -p $(BLDDIR)/pkgbuild/$(APPNAME)
	mkdir -p $(BLDDIR)/$(APPNAME)
	cp copyright $(BLDDIR)
	cp depend.$(platform) $(BLDDIR)/depend
	cat pkginfo.tmpl | sed s/{__VERSION}/$(VERSION)/g | sed s/{__BUILD}/$(BUILD)/g \
		| sed s/{__APPNAME}/$(APPNAME)/g | sed s/{__PKGNAME}/$(APPNAME)/g \
		| sed s/{__PKGARCH}/$(PKGARCH)/g >$(BLDDIR)/pkginfo
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(APPNAME) docdir=$(BLDDIR)/pkgbuild/$(APPNAME)/doc M32=yes install
	$(MAKE) prefix=$(BLDDIR)/pkgbuild/$(APPNAME) docdir=$(BLDDIR)/pkgbuild/$(APPNAME)/doc install
	sh makeproto $(BLDDIR)/pkgbuild/$(APPNAME) $(platform) $(subplatform) > $(BLDDIR)/rr.proto
	cd $(BLDDIR); \
	pkgmk -o -r ./pkgbuild -d . -a `uname -p` -f rr.proto; \
	rm rr.proto copyright depend pkginfo; \
	pkgtrans -s `pwd` `pwd`/$(APPNAME).pkg $(APPNAME)
	bzip2 $(BLDDIR)/$(APPNAME).pkg
	rm -rf $(BLDDIR)/pkgbuild
	rm -rf $(BLDDIR)/$(APPNAME)

ifeq ($(platform), osxx86)
ifeq ($(subplatform), 64)
dist: rr rr32 diags diags32 mesademos mesademos32
	sh makemacpkg $(APPNAME) $(BLDDIR) $(VERSION) $(BUILD) $(EDIR) $(EDIR32) universal
else
dist: rr diags mesademos
	sh makemacpkg $(APPNAME) $(BLDDIR) $(VERSION) $(BUILD) $(EDIR) $(EDIR32)
endif
endif
ifeq ($(platform), osx)
dist: rr rr32 diags diags32 mesademos mesademos32
	sh makemacpkg $(APPNAME) $(BLDDIR) $(VERSION) $(BUILD) $(EDIR) $(EDIR32)
endif

.PHONY: tarball
tarball:
	rm -rf $(BLDDIR)/fakeroot
	rm -f $(BLDDIR)/$(APPNAME).tar.gz
	$(MAKE) prefix=$(BLDDIR)/fakeroot/VirtualGL-$(VERSION) install
	cd $(BLDDIR)/fakeroot; \
	tar cf ../$(APPNAME).tar *
	gzip $(BLDDIR)/$(APPNAME).tar
	rm -rf $(BLDDIR)/fakeroot

##########################################################################
endif
##########################################################################
