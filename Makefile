all: rr mesademos

.PHONY: rr util jpeg mesademos clean

rr: util jpeg

rr util jpeg mesademos:
	cd $@; $(MAKE); cd ..

clean:
	cd rr; $(MAKE) clean; cd ..; \
	cd util; $(MAKE) clean; cd ..; \
	cd jpeg; $(MAKE) clean; cd ..; \
	cd mesademos; $(MAKE) clean; cd ..

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

dist: rr
	$(RM) $(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DEDIR=$(WEDIR) rr.nsi


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

PACKAGENAME = $(APPNAME)
ifeq ($(subplatform), 64)
PACKAGENAME = $(APPNAME)64
endif

ifeq ($(subplatform), 64)
install: rr
	mkdir -p $(prefix)/bin
	mkdir -p $(prefix)/lib64
	install -m 755 $(EDIR)/rrlaunch64 $(prefix)/bin/rrlaunch64
	install -m 755 $(LDIR)/libhpjpeg.so $(prefix)/lib64/libhpjpeg.so
	install -m 755 $(LDIR)/librrfaker.so $(prefix)/lib64/librrfaker.so
	echo Install complete.
else
install: rr
	mkdir -p $(prefix)/bin
	mkdir -p $(prefix)/lib
	install -m 755 rr/rrxclient.sh $(prefix)/bin/rrxclient_daemon
	install -m 755 rr/rrxclient_ssl.sh $(prefix)/bin/rrxclient_ssldaemon
	install -m 755 rr/rrxclient_config $(prefix)/bin/rrxclient_config
	install -m 644 rr/rrcert.cnf /etc/rrcert.cnf
	install -m 755 rr/newrrcert $(prefix)/bin/newrrcert
	install -m 755 $(EDIR)/rrlaunch $(prefix)/bin/rrlaunch
	install -m 755 $(EDIR)/rrxclient $(prefix)/bin/rrxclient
	install -m 755 $(LDIR)/libhpjpeg.so $(prefix)/lib/libhpjpeg.so
	install -m 755 $(LDIR)/librrfaker.so $(prefix)/lib/librrfaker.so
	echo Install complete.
endif

ifeq ($(subplatform), 64)
uninstall:
	$(RM) $(prefix)/bin/rrlaunch64
	$(RM) $(prefix)/lib64/libhpjpeg.so
	$(RM) $(prefix)/lib64/librrfaker.so
	echo Uninstall complete.
else
uninstall:
	$(prefix)/bin/rrxclient_daemon stop || echo Client not installed
	$(prefix)/bin/rrxclient_ssldaemon stop || echo Secure client not installed
	$(RM) $(prefix)/bin/rrxclient_daemon
	$(RM) $(prefix)/bin/rrxclient_ssldaemon
	$(RM) $(prefix)/bin/rrxclient_config
	$(RM) /etc/rrcert.cnf
	$(RM) $(prefix)/bin/newrrcert
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/bin/rrxclient
	$(RM) $(prefix)/lib/libhpjpeg.so
	$(RM) $(prefix)/lib/librrfaker.so
	echo Uninstall complete.
endif

dist: rr rpms/BUILD rpms/RPMS
	rm $(PACKAGENAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_curdir `pwd`" --define "_topdir `pwd`/rpms" \
		--define "_version $(VERSION)" --define "_build $(BUILD)" --define "_bindir $(EDIR)" \
		--define "_libdir $(LDIR)" --define "_appname $(APPNAME)" --target $(RPMARCH) \
		rr.spec; \
	mv rpms/RPMS/$(RPMARCH)/$(PACKAGENAME)-$(VERSION)-$(BUILD).$(RPMARCH).rpm $(PACKAGENAME).$(RPMARCH).rpm

rpms/BUILD:
	mkdir -p rpms/BUILD

rpms/RPMS:
	mkdir -p rpms/RPMS

##########################################################################
endif
##########################################################################
