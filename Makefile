all: rr mesademos diags

.PHONY: rr util jpeg mesademos diags clean

rr: util jpeg

rr util jpeg mesademos diags:
	cd $@; $(MAKE); cd ..

clean:
	cd rr; $(MAKE) clean; cd ..; \
	cd util; $(MAKE) clean; cd ..; \
	cd jpeg; $(MAKE) clean; cd ..; \
	cd mesademos; $(MAKE) clean; cd ..; \
	cd diags; $(MAKE) clean; cd ..

TOPDIR=.
include Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

ifeq ($(DEBUG), yes)
WEDIR := $(platform)$(subplatform)\\bind
else
WEDIR := $(platform)$(subplatform)\\bin
endif

dist: rr diags
	$(RM) $(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DEDIR=$(WEDIR) rr.nsi
	$(RM) $(APPNAME)-diags.exe
	makensis //DAPPNAME=$(APPNAME)-diags //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DEDIR=$(WEDIR) rrdiags.nsi

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

RRLAUNCH = rrlaunch
PACKAGENAME = $(APPNAME)
ifeq ($(subplatform), 64)
RRLAUNCH = rrlaunch64
PACKAGENAME = $(APPNAME)64
endif

install: rr
	install -m 755 rr/rrxclient.sh /etc/rc.d/init.d/rrxclient
	install -m 644 rr/rrcert.cnf /etc/rrcert.cnf
	install -m 755 rr/newrrcert $(prefix)/bin/newrrcert
	install -m 755 $(EDIR)/$(RRLAUNCH) $(prefix)/bin/$(RRLAUNCH)
	install -m 755 $(EDIR)/rrxclient $(prefix)/bin/rrxclient
	install -m 755 $(LDIR)/libhpjpeg.so $(prefix)/lib/libhpjpeg.so
	install -m 755 $(LDIR)/librrfaker.so $(prefix)/lib/librrfaker.so
	echo Install complete.

uninstall:
	/etc/rc.d/init.d/rrxclient stop || echo RRXClient not installed
	chkconfig --del rrxclient || echo RRXClient service not registered
	$(RM) /etc/rc.d/init.d/rrxclient
	$(RM) /etc/rrcert.cnf
	$(RM) $(prefix)/bin/newrrcert
	$(RM) $(prefix)/bin/$(RRLAUNCH)
	$(RM) $(prefix)/bin/rrxclient
	$(RM) $(prefix)/lib/libhpjpeg.so
	$(RM) $(prefix)/lib/librrfaker.so
	echo Uninstall complete.

dist: rr diags rpms/BUILD rpms/RPMS
	rm $(PACKAGENAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_curdir `pwd`" --define "_topdir `pwd`/rpms" \
		--define "_version $(VERSION)" --define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_libdir $(LDIR)" --define "_appname $(APPNAME)" --target $(RPMARCH) \
		rr.spec
	mv rpms/RPMS/$(RPMARCH)/$(PACKAGENAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(PACKAGENAME).$(RPMARCH).rpm
	rpmbuild -bb --define "_curdir `pwd`" --define "_topdir `pwd`/rpms" \
		--define "_version $(VERSION)" --define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_libdir $(LDIR)" --define "_appname $(APPNAME)" --target $(RPMARCH) \
		rrdiags.spec
	mv rpms/RPMS/$(RPMARCH)/$(PACKAGENAME)-diags-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(PACKAGENAME)-diags.$(RPMARCH).rpm

rpms/BUILD:
	mkdir -p rpms/BUILD

rpms/RPMS:
	mkdir -p rpms/RPMS

##########################################################################
endif
##########################################################################
