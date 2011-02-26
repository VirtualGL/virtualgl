include ../Makerules
include ./Makerules.tjpeg

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe

OBJS = $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS)

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/rrutil.lib
	$(LINK) $(LDFLAGS) $< -out:$@ $(JPEGLINK) rrutil.lib
	
$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj
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

ifneq ($(JPEGLIB), libjpeg-turbo)
 TARGETS := $(LTARGET) $(TARGETS)
endif


all: $(TARGETS)
lib: $(LTARGET)

clean:
	-$(RM) $(TARGETS) $(LTARGET) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)


$(ODIR)/turbojpeg.o: ../include/turbojpeg.h

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
