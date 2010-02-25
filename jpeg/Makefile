include ../Makerules
include ./Makerules.tjpeg

##########################################################################
ifeq ($(platform), windows)
##########################################################################

LTARGET = $(LDIR)/tjpeg.lib

TARGETS = $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe

OBJS = $(ODIR)/turbojpeg.obj \
       $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj

ifeq ($(JPEGLIB), libjpeg)
 LTARGET := libjpeg $(LTARGET)
endif

ifneq ($(JPEGLIB), ipp)
 TARGETS := $(LTARGET) $(TARGETS)
 LDEP := $(LTARGET)
endif


all: $(TARGETS)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS)
	cd jpeg-6b; $(MAKE) clean; cd ..

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -Fo$@

endif

ifeq ($(JPEGLIB), libjpeg-turbo)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) $(CFLAGS) -c $< -Fo$@

endif

ifeq ($(JPEGLIB), medialib)

$(ODIR)/turbojpeg.obj: turbojpeg-mlib.c jpeg_c_EncoderHuffman.c jpeg_c_DecoderHuffman.c jpeg_IN.h
	$(CC) $(CFLAGS) -c $< -Fo$@

endif

$(LDIR)/tjpeg.lib: $(ODIR)/turbojpeg.obj
	$(AR) -OUT:$@ $^

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/rrutil.lib $(JPEGDEP)
	$(LINK) $(LDFLAGS) $< -out:$@ $(JPEGLINK) rrutil.lib

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(JPEGDEP)
	$(LINK) $(LDFLAGS) $< -out:$@ $(JPEGLINK)

##########################################################################
else
##########################################################################

LTARGET = $(LDIR)/libtjpeg.a

TARGETS = $(EDIR)/jpgtest \
          $(EDIR)/jpegut

OBJS = $(ODIR)/turbojpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o

ifeq ($(JPEGLIB), libjpeg)
 LTARGET := libjpeg $(LTARGET)
endif

ifneq ($(JPEGLIB), ipp)
 TARGETS := $(LTARGET) $(TARGETS)
endif


all: $(TARGETS)
lib: $(LTARGET)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS) $(OBJS:.o=.d)
	cd jpeg-6b; \
	$(MAKE) clean

-include $(OBJS:.o=.d)


$(ODIR)/turbojpeg.o: ../include/turbojpeg.h

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.o: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -o $@

endif

ifeq ($(JPEGLIB), libjpeg-turbo)
ifeq ($(platform), cygwin)

$(ODIR)/turbojpeg.o: turbojpegl.c
	$(CC) $(CFLAGS) -c $< -o $@

else

$(ODIR)/turbojpeg.o: turbojpegl.c
	$(CC) $(CFLAGS) -I$(LJTDIR)/include -c $< -o $@

endif
endif

ifeq ($(JPEGLIB), quicktime)

$(ODIR)/turbojpeg.o: turbojpegqt.c
	$(CC)  $(CFLAGS) -c $< -o $@

endif

ifeq ($(JPEGLIB), medialib)

$(ODIR)/turbojpeg.o: turbojpeg-mlib.c jpeg_c_EncoderHuffman.c jpeg_c_DecoderHuffman.c jpeg_IN.h
	$(CC) $(CFLAGS) -c $< -o $@

endif


$(LDIR)/libtjpeg.a: $(ODIR)/turbojpeg.o
	$(AR) $@ $<

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(JPEGDEP) $(LDIR)/librrutil.a
	$(CXX) $(LDFLAGS) $< -o $@ $(JPEGLINK) -lrrutil

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(JPEGDEP)
	$(CC) $(LDFLAGS) $< -o $@ $(JPEGLINK)

##########################################################################
endif
##########################################################################
