include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

LTARGET = $(EDIR)/turbojpeg.dll \
          $(LDIR)/turbojpeg.lib

TARGETS = $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe

OBJS = $(ODIR)/turbojpeg.obj \
       $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj

ifeq ($(JPEGLIB), libjpeg)
LTARGET := libjpeg $(LTARGET)
endif

ifeq ($(JPEGLIB), ipp)
LDEP =
else
TARGETS := $(TARGETS) $(LTARGET)
LDEP := $(LTARGET)
endif


all: $(TARGETS)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; \
	diff -q jconfig.vc jconfig.h || cp jconfig.vc jconfig.h; \
	$(MAKE) -f makefile.vc; cd ..

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS)
	cd jpeg-6b; $(MAKE) -f makefile.vc clean; cd ..

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


ifeq ($(JPEGLIB), pegasus)

PEGDIR = ../../pictools

$(ODIR)/turbojpeg.obj: turbojpegp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -I$(PEGDIR)/include -DWINDOWS -D__FLAT__ -c $< -Fo$@

JPEGLINK = -LIBPATH:$(PEGDIR)/lib picnm.lib

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

endif

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.obj: turbojpegqt.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = qtmlClient.lib user32.lib advapi32.lib

endif

$(EDIR)/turbojpeg.dll $(LDIR)/turbojpeg.lib: $(ODIR)/turbojpeg.obj $(JPEGDEP)
	$(LINK) $(LDFLAGS) -dll $< -out:$(EDIR)/turbojpeg.dll \
		-implib:$(LDIR)/turbojpeg.lib $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDEP) $(LDIR)/rrutil.lib
	$(LINK) $(LDFLAGS) $< -out:$@ turbojpeg.lib rrutil.lib

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(LDEP)
	$(LINK) $(LDFLAGS) $< -out:$@ turbojpeg.lib

##########################################################################
else
##########################################################################

ifeq ($(platform), cygwin)
LTARGET = $(EDIR)/turbojpeg.dll \
          $(LDIR)/turbojpeg.lib
else
LTARGET = $(LDIR)/libturbojpeg.$(SHEXT)
endif

TARGETS = $(EDIR)/jpgtest \
          $(EDIR)/jpegut

OBJS = $(ODIR)/turbojpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o

ifeq ($(JPEGLIB), libjpeg)
LTARGET := libjpeg $(LTARGET)
endif

ifeq ($(JPEGLIB), ipp)
LDEP =
else
TARGETS := $(TARGETS) $(LTARGET)
LDEP := $(LTARGET)
endif


all: $(TARGETS)
lib: $(LTARGET)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; sh configure CC=$(CC); $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS) $(OBJS:.o=.d) $(ODIR)/turbojpeg-mapfile.E
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

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.o: turbojpegqt.c
	$(CC)  $(CFLAGS) -c $< -o $@

JPEGLINK = -framework ApplicationServices -framework CoreFoundation -framework CoreServices -framework QuickTime

endif

ifeq ($(JPEGLIB), medialib)

$(ODIR)/turbojpeg.o: turbojpeg-mlib.c jpeg_c_EncoderHuffman.c jpeg_c_DecoderHuffman.c jpeg_IN.h
	$(CC) $(CFLAGS) -c $< -o $@

JPEGLINK = -lmlib

endif

$(EDIR)/turbojpeg.dll $(LDIR)/turbojpeg.lib $(LDIR)/turbojpeg.def: $(ODIR)/turbojpeg.o $(JPEGDEP)
	$(CC) $(LDFLAGS) -shared $< -o $@ -Wl,-out-implib,$(LDIR)/turbojpeg.lib \
		-Wl,--output-def,$(LDIR)/turbojpeg.def $(JPEGLINK)

ifneq ($(platform), osx)
ifneq ($(platform), osxx86)
JPEGLINK := $(JPEGLINK) $(MAPFLAG)$(ODIR)/turbojpeg-mapfile.E
endif
endif

$(LDIR)/libturbojpeg.$(SHEXT): $(ODIR)/turbojpeg.o $(ODIR)/turbojpeg-mapfile.E $(JPEGDEP)
	$(CC) $(LDFLAGS) $(SHFLAG) $< -o $@ $(JPEGLINK)

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDEP) $(LDIR)/librrutil.a
	$(CXX) $(LDFLAGS) $< -o $@ -lturbojpeg -lrrutil

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(LDEP)
	$(CC) $(LDFLAGS) $< -o $@ -lturbojpeg

##########################################################################
endif
##########################################################################
