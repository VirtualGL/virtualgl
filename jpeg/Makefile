include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(EDIR)/hpjpeg.dll \
          $(LDIR)/hpjpeg.lib \
          $(EDIR)/jpgtest.exe

OBJS = $(ODIR)/hpjpeg.obj \
       $(ODIR)/jpgtest.obj

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

else

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/hpjpeg.obj: hpjpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

else

IPPLINK = ippjmerged.lib ippimerged.lib ippsmerged.lib ippjemerged.lib \
	ippiemerged.lib ippsemerged.lib ippcorel.lib

$(ODIR)/hpjpeg.obj: hpjpegipp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(IPPLINK)

endif
endif

$(EDIR)/hpjpeg.dll $(LDIR)/hpjpeg.lib: $(ODIR)/hpjpeg.obj $(JPEGDEP)
	$(LINK) -dll -out:$(EDIR)/hpjpeg.dll -implib:$(LDIR)/hpjpeg.lib $< $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/hputil.lib $(LDIR)/hpjpeg.lib
	$(LINK) $< -OUT:$@ $(HPULIB) hpjpeg.lib

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libhpjpeg.so \
          $(EDIR)/jpgtest

OBJS = $(ODIR)/hpjpeg.o \
       $(ODIR)/jpgtest.o

ifeq ($(JPEGLIB), libjpeg)
TARGETS := libjpeg $(TARGETS)
endif


all: $(TARGETS)

.PHONY: libjpeg
libjpeg:
	cd jpeg-6b; configure CC=$(CC); $(MAKE); cd ..

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)
	cd jpeg-6b; \
	$(MAKE) clean || (configure CC=$(CC); $(MAKE) clean); cd ..

-include $(OBJS:.o=.d)


ifeq ($(JPEGLIB), pegasus)

PEGDIR = ../../pictools

$(ODIR)/hpjpeg.o: hpjpegp.c
	$(CC) $(CFLAGS) -I$(PEGDIR)/include -c $< -o $@

JPEGLINK = -L$(PEGDIR)/lib -lpicl20

else

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/hpjpeg.o: hpjpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -o $@

JPEGLINK = $(LDIR)/libjpeg.a
JPEGDEP = $(LDIR)/libjpeg.a

else

ifeq ($(IPPDIR),)
IPPDIR = /opt/intel/ipp40
endif
IPPLINK = -L$(IPPDIR)/lib -lippcore \
        -lippjemerged -lippiemerged -lippsemerged \
        -lippjmerged -lippimerged -lippsmerged

$(ODIR)/hpjpeg.o: hpjpegipp.c
	$(CC) -I$(IPPDIR)/include $(CFLAGS) -c $< -o $@

JPEGLINK = $(IPPLINK)

endif
endif

$(LDIR)/libhpjpeg.so: $(ODIR)/hpjpeg.o $(JPEGDEP)
	$(CC) $(LDFLAGS) -shared $< -o $@ $(JPEGLINK)

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDIR)/libhputil.a $(LDIR)/libhpjpeg.so
	$(CC) $(LDFLAGS) $< -o $@ -lhputil -lhpjpeg

##########################################################################
endif
##########################################################################
