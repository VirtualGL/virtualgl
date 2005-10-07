all: rr mesademos diags

.PHONY: rr util jpeg mesademos diags clean test

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

test: rr mesademos
	chmod u+x mesademos/dotests ;\
	exec mesademos/dotests $(EDIR) $(platform) $(subplatform)

##########################################################################
ifeq ($(platform), windows)
##########################################################################

ifeq ($(DEBUG), yes)
WEDIR := $(platform)$(subplatform)\\dbg\\bin
else
WEDIR := $(platform)$(subplatform)\\bin
endif

.PHONY: dist
dist: rr diags
	$(RM) $(APPNAME).exe
	makensis //DAPPNAME=$(APPNAME) //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DEDIR=$(WEDIR) rr.nsi || \
	makensis /DAPPNAME=$(APPNAME) /DVERSION=$(VERSION) \
		/DBUILD=$(BUILD) /DEDIR=$(WEDIR) rr.nsi  # Cygwin doesn't like the //


##########################################################################
else
##########################################################################

ifeq ($(prefix),)
prefix=/usr/local
endif

.PHONY: install uninstall
ifeq ($(subplatform), 64)
install: rr
	mkdir -p $(prefix)/bin
	mkdir -p $(prefix)/lib64
	install -m 755 $(EDIR)/rrlaunch $(prefix)/bin/rrlaunch
	install -m 755 $(LDIR)/libturbojpeg.so $(prefix)/lib64/libturbojpeg.so
	install -m 755 $(LDIR)/librrfaker.so $(prefix)/lib64/librrfaker.so
	echo Install complete.
else
install: rr
	mkdir -p $(prefix)/bin
	mkdir -p $(prefix)/lib
	install -m 755 rr/rrxclient.sh $(prefix)/bin/rrxclient_daemon
	install -m 755 rr/rrxclient_ssl.sh $(prefix)/bin/rrxclient_ssldaemon
	install -m 755 rr/rrxclient_config $(prefix)/bin/rrxclient_config
	install -m 755 $(EDIR)/rrlaunch $(prefix)/bin/rrlaunch
	install -m 755 $(EDIR)/rrxclient $(prefix)/bin/rrxclient
	install -m 755 $(LDIR)/libturbojpeg.so $(prefix)/lib/libturbojpeg.so
	install -m 755 $(LDIR)/librrfaker.so $(prefix)/lib/librrfaker.so
	echo Install complete.
endif

ifeq ($(subplatform), 64)
uninstall:
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/lib64/libturbojpeg.so
	$(RM) $(prefix)/lib64/librrfaker.so
	echo Uninstall complete.
else
uninstall:
	$(prefix)/bin/rrxclient_daemon stop || echo Client not installed
	$(prefix)/bin/rrxclient_ssldaemon stop || echo Secure client not installed
	$(RM) $(prefix)/bin/rrxclient_daemon
	$(RM) $(prefix)/bin/rrxclient_ssldaemon
	$(RM) $(prefix)/bin/rrxclient_config
	$(RM) $(prefix)/bin/rrlaunch
	$(RM) $(prefix)/bin/rrxclient
	$(RM) $(prefix)/lib/libturbojpeg.so
	$(RM) $(prefix)/lib/librrfaker.so
	echo Uninstall complete.
endif

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

.PHONY: dist
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

##########################################################################
endif
##########################################################################
