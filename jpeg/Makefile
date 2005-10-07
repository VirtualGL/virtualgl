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
	$(CC) $(CFLAGS) -DDLLDEFINE -I$(PEGDIR)/include -DWINDOWS -D__FLAT__ -c $< -Fo$@

JPEGLINK = -libpath:$(PEGDIR)/lib picnm.lib

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

endif

ifeq ($(JPEGLIB), ipp)

IPPLINK = ippjmerged.lib ippimerged.lib ippsmerged.lib ippjemerged.lib \
	ippiemerged.lib ippsemerged.lib ippcorel.lib

$(ODIR)/turbojpeg.obj: turbojpegipp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(IPPLINK)

endif

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.obj: turbojpegqt.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = qtmlClient.lib user32.lib advapi32.lib

endif

$(EDIR)/turbojpeg.dll $(LDIR)/turbojpeg.lib: $(ODIR)/turbojpeg.obj $(JPEGDEP)
	$(LINK) -dll -out:$(EDIR)/turbojpeg.dll -implib:$(LDIR)/turbojpeg.lib $< $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/turbojpeg.lib $(LDIR)/rrutil.lib
	$(LINK) $< -OUT:$@ turbojpeg.lib rrutil.lib

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(LDIR)/turbojpeg.lib
	$(LINK) $< -OUT:$@ turbojpeg.lib

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libturbojpeg.$(SHEXT) \
          $(EDIR)/jpgtest \
          $(EDIR)/jpegut

OBJS = $(ODIR)/turbojpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o

ifeq ($(JPEGLIB),)
JPEGLIB = $(DEFAULTJPEGLIB)
endif

ifeq ($(JPEGLIB), libjpeg)
TARGETS := libjpeg $(TARGETS)
endif


all: $(TARGETS)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; sh configure CC=$(CC); $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)
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

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDIR)/libturbojpeg.$(SHEXT) $(LDIR)/librrutil.a
	$(CXX) $(LDFLAGS) $< -o $@ -lturbojpeg -lrrutil

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(LDIR)/libturbojpeg.$(SHEXT)
	$(CC) $(LDFLAGS) $< -o $@ -lturbojpeg

ifeq ($(platform), linux)

ifeq ($(subplatform),)
RPMARCH = i386
EDIR32 := $(EDIR)
LDIR32 := $(LDIR)
all32:
else
RPMARCH = $(ARCH)
ifneq ($(DISTRO),)
EDIR32 := $(TOPDIR)/$(platform)/$(DISTRO)/bin
LDIR32 := $(TOPDIR)/$(platform)/$(DISTRO)/lib
else
EDIR32 := $(TOPDIR)/$(platform)/bin
LDIR32 := $(TOPDIR)/$(platform)/lib
endif
all32:
	$(MAKE) M32=yes
endif

TJPEGBUILD = 1
ifeq ($(JPEGLIB), ipp)
TJPEGBUILD = 1ipp
endif
ifeq ($(JPEGLIB), pegasus)
TJPEGBUILD = 1peg
endif

dist: all all32
	if [ -d $(BLDDIR)/rpms ]; then rm -rf $(BLDDIR)/rpms; fi
	mkdir -p $(BLDDIR)/rpms/RPMS
	ln -fs `pwd` $(BLDDIR)/rpms/BUILD
	rm -f $(BLDDIR)/$(PACKAGENAME).$(RPMARCH).rpm; \
	rpmbuild -bb --define "_blddir `pwd`/$(BLDDIR)" --define "_topdir $(BLDDIR)/rpms" \
		--define "_bindir $(EDIR)" --define "_bindir32 $(EDIR32)" --define "_build $(TJPEGBUILD)" \
		--define "_libdir $(LDIR)" --define "_libdir32 $(LDIR32)" --target $(RPMARCH) \
		turbojpeg.spec; \
	mv $(BLDDIR)/rpms/RPMS/$(RPMARCH)/turbojpeg-1.0-$(TJPEGBUILD).$(RPMARCH).rpm $(BLDDIR)
	rm -rf $(BLDDIR)/rpms

endif

##########################################################################
endif
##########################################################################
