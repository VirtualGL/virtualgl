include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(EDIR)/hpjpeg.dll \
          $(LDIR)/hpjpeg.lib \
          $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe \
          $(EDIR)/24to32.exe

OBJS = $(ODIR)/hpjpeg.obj \
       $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj \
       $(ODIR)/24to32.obj

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

$(ODIR)/hpjpeg.obj: hpjpegp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -I$(PEGDIR)/include -DWINDOWS -D__FLAT__ -c $< -Fo$@

JPEGLINK = -libpath:$(PEGDIR)/lib picnm.lib

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/hpjpeg.obj: hpjpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

endif

ifeq ($(JPEGLIB), ipp)

IPPLINK = ippjmerged.lib ippimerged.lib ippsmerged.lib ippjemerged.lib \
	ippiemerged.lib ippsemerged.lib ippcorel.lib

$(ODIR)/hpjpeg.obj: hpjpegipp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(IPPLINK)

endif

$(EDIR)/hpjpeg.dll $(LDIR)/hpjpeg.lib: $(ODIR)/hpjpeg.obj $(JPEGDEP)
	$(LINK) -dll -out:$(EDIR)/hpjpeg.dll -implib:$(LDIR)/hpjpeg.lib $< $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/hpjpeg.lib
	$(LINK) $< -OUT:$@ hpjpeg.lib

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(LDIR)/hpjpeg.lib
	$(LINK) $< -OUT:$@ hpjpeg.lib

$(EDIR)/24to32.exe: $(ODIR)/24to32.obj
	$(LINK) $< -OUT:$@

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libhpjpeg.so \
          $(EDIR)/jpgtest \
          $(EDIR)/jpegut \
          $(EDIR)/24to32

OBJS = $(ODIR)/hpjpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o \
       $(ODIR)/24to32.o

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

$(ODIR)/hpjpeg.o: hpjpegp.c ../include/hpjpeg.h
	$(CC) $(CFLAGS) -I$(PEGDIR)/include -c $< -o $@

JPEGLINK = -L$(PEGDIR)/lib -lpicl20

endif

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/hpjpeg.o: hpjpegl.c ../include/hpjpeg.h
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -o $@

JPEGLINK = $(LDIR)/libjpeg.a
JPEGDEP = $(LDIR)/libjpeg.a

endif

ifeq ($(JPEGLIB), ipp)

ifeq ($(IPPDIR),)
IPPDIR = /opt/intel/ipp40
endif
IPPLINK = -L$(IPPDIR)/lib -lippcore \
        -lippjemerged -lippiemerged -lippsemerged \
        -lippjmerged -lippimerged -lippsmerged

$(ODIR)/hpjpeg.o: hpjpegipp.c ../include/hpjpeg.h
	$(CC) -I$(IPPDIR)/include $(CFLAGS) -c $< -o $@

JPEGLINK = $(IPPLINK)

endif

$(LDIR)/libhpjpeg.so: $(ODIR)/hpjpeg.o $(JPEGDEP)
	$(CC) $(LDFLAGS) $(SHFLAG) $< -o $@ $(JPEGLINK)

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDIR)/libhpjpeg.so
	$(CXX) $(LDFLAGS) $< -o $@ -lhpjpeg

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(LDIR)/libhpjpeg.so
	$(CC) $(LDFLAGS) $< -o $@ -lhpjpeg

$(EDIR)/24to32: $(ODIR)/24to32.o
	$(CC) $(LDFLAGS) $< -o $@

##########################################################################
endif
##########################################################################
