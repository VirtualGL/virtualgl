include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(EDIR)/turbojpeg.dll \
          $(LDIR)/turbojpeg.lib \
          $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe

OBJS = $(ODIR)/turbojpeg.obj \
       $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj

ifeq ($(JPEGLIB),)
JPEGLIB = $(DEFAULTJPEGLIB)
endif

ifeq ($(JPEGLIB), libjpeg)
TARGETS := libjpeg $(TARGETS)
endif


all: $(TARGETS)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; \
	diff -q jconfig.vc jconfig.h || cp jconfig.vc jconfig.h; \
	$(MAKE) -f makefile.vc; cd ..

clean:
	-$(RM) $(TARGETS) $(OBJS)
	cd jpeg-6b; $(MAKE) -f makefile.vc clean; cd ..

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


ifeq ($(JPEGLIB), pegasus)

PEGDIR = ../../pictools

$(ODIR)/turbojpeg.obj: turbojpegp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -I$(PEGDIR)/include -DWINDOWS -D__FLAT__ -c $< -o $@

JPEGLINK = -L$(PEGDIR)/lib -lpicnm

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -o $@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

endif

ifeq ($(JPEGLIB), ipp)

IPPLINK = -lippjemerged	-lippiemerged -lippsemerged \
	-lippjmerged -lippimerged -lippsmerged -lippcorel

$(ODIR)/turbojpeg.obj: turbojpegipp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -o $@

JPEGLINK = $(IPPLINK)

endif

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.obj: turbojpegqt.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -o $@

JPEGLINK = qtmlClient.lib user32.lib advapi32.lib

endif

$(EDIR)/turbojpeg.dll $(LDIR)/turbojpeg.lib: $(ODIR)/turbojpeg.obj $(JPEGDEP)
	$(CC) $(LDFLAGS) -shared $< -o $@ -Wl,-out-implib,$(LDIR)/turbojpeg.lib \
		-Wl,--output-def,$(LDIR)/turbojpeg.def $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/turbojpeg.lib $(LDIR)/rrutil.lib
	$(CXX) $(LDFLAGS) $< -o $@ -lturbojpeg -lrrutil

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(LDIR)/turbojpeg.lib
	$(CC) $(LDFLAGS) $< -o  $@ -lturbojpeg

##########################################################################
else
##########################################################################

LTARGET = $(LDIR)/libturbojpeg.$(SHEXT)

TARGETS = $(EDIR)/jpgtest \
          $(EDIR)/jpegut

ifeq ($(platform), linux)
LDEP =
else
TARGETS := $(TARGETS) $(LTARGET)
LDEP := $(LTARGET)
endif

OBJS = $(ODIR)/turbojpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o

ifeq ($(JPEGLIB),)
JPEGLIB = $(DEFAULTJPEGLIB)
endif

ifeq ($(JPEGLIB), libjpeg)
LTARGET := libjpeg $(LTARGET)
endif


all: $(TARGETS)
lib: $(LTARGET)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; sh configure CC=$(CC); $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS) $(OBJS:.o=.d)
	cd jpeg-6b; \
	$(MAKE) clean || (sh configure CC=$(CC); $(MAKE) clean); cd ..

-include $(OBJS:.o=.d)


ifeq ($(JPEGLIB), pegasus)

PEGDIR = ../../pictools

$(ODIR)/turbojpeg.o: turbojpegp.c ../include/turbojpeg.h
	$(CC) $(CFLAGS) -I$(PEGDIR)/include -c $< -o $@

JPEGLINK = -L$(PEGDIR)/lib -lpicl20

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.o: turbojpegl.c ../include/turbojpeg.h
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -o $@

JPEGLINK = $(LDIR)/libjpeg.a
JPEGDEP = $(LDIR)/libjpeg.a

endif

ifeq ($(JPEGLIB), ipp)

ifeq ($(IPPDIR),)
IPPDIR = /opt/intel/ipp41/ia32_itanium
ifeq ($(subplatform), 64)
IPPDIR = /opt/intel/ipp41/em64t
endif
endif
IPPLINK = -L$(IPPDIR)/lib -lippcore \
        -lippjemerged -lippiemerged -lippsemerged \
        -lippjmerged -lippimerged -lippsmerged
ifeq ($(IPPSHARED), yes)
IPPLINK = -L$(IPPDIR)/sharedlib \
        -lippj -lippi -lipps -lippcore
endif
ifeq ($(subplatform), 64)
IPPLINK = -L$(IPPDIR)/lib \
        -lippjemergedem64t -lippjmergedem64t -lippiemergedem64t \
        -lippimergedem64t -lippsemergedem64t -lippsmergedem64t \
        -lippcoreem64t
ifeq ($(IPPSHARED), yes)
IPPLINK = -L$(IPPDIR)/sharedlib \
        -lippjem64t -lippiem64t -lippsem64t -lippcoreem64t
endif
endif
ifeq ($(subplatform), ia64)
IPPLINK = -L$(IPPDIR)/lib \
        -lippji7 -lippii7 -lippsi7 -lippcore64
endif

$(ODIR)/turbojpeg.o: turbojpegipp.c ../include/turbojpeg.h
	$(CC) -I$(IPPDIR)/include $(CFLAGS) -c $< -o $@

JPEGLINK = $(IPPLINK)

endif

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.o: turbojpegqt.c
	$(CC)  $(CFLAGS) -c $< -o $@

JPEGLINK = -framework ApplicationServices -framework CoreFoundation -framework CoreServices -framework QuickTime

endif

ifeq ($(JPEGLIB), medialib)

$(ODIR)/turbojpeg.o: turbojpeg-mlib.c turbojpeg-mlibhuff.c
	$(CC) $(CFLAGS) -c $< -o $@

JPEGLINK = -lmlib

endif

$(LDIR)/libturbojpeg.$(SHEXT): $(ODIR)/turbojpeg.o $(JPEGDEP)
	$(CC) $(LDFLAGS) $(SHFLAG) $< -o $@ $(JPEGLINK)

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDEP) $(LDIR)/librrutil.a
	$(CXX) $(LDFLAGS) $< -o $@ -lturbojpeg -lrrutil

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(LDEP)
	$(CC) $(LDFLAGS) $< -o $@ -lturbojpeg

ifeq ($(platform), linux)

ifeq ($(subplatform),)
lib32:
else
lib32:
	$(MAKE) M32=yes lib
endif

TJPEGBUILD = 1
ifeq ($(JPEGLIB), ipp)
TJPEGBUILD = 1ipp
endif
ifeq ($(JPEGLIB), pegasus)
TJPEGBUILD = 1peg
endif

dist: lib lib32
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	ln -fs `pwd` $(BLDDIR)/rpms/BUILD
	rm -f $(BLDDIR)/$(PACKAGENAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir `pwd`/$(BLDDIR)" --define "_topdir $(BLDDIR)/rpms" \
		--define "_bindir $(EDIR)" --define "_bindir32 $(EDIR32)" --define "_build $(TJPEGBUILD)" \
		--define "_libdir $(LDIR)" --define "_libdir32 $(LDIR32)" --target $(RPMARCH) \
		turbojpeg.spec; \
	mv $(BLDDIR)/rpms/RPMS/$(RPMARCH)/turbojpeg-1.01-$(TJPEGBUILD).$(RPMARCH).rpm $(BLDDIR)
	rm -rf $(BLDDIR)/rpms

endif

##########################################################################
endif
##########################################################################
