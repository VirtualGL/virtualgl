include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(EDIR)/turbojpeg.dll \
          $(LDIR)/turbojpeg.lib \
          $(EDIR)/jpgtest.exe \
          $(EDIR)/jpegut.exe \
          $(EDIR)/24to32.exe

OBJS = $(ODIR)/turbojpeg.obj \
       $(ODIR)/jpgtest.obj \
       $(ODIR)/jpegut.obj \
       $(ODIR)/24to32.obj

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

else

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.obj: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(LDIR)/libjpeg.lib
JPEGDEP = $(LDIR)/libjpeg.lib

else

IPPLINK = ippjmerged.lib ippimerged.lib ippsmerged.lib ippjemerged.lib \
	ippiemerged.lib ippsemerged.lib ippcorel.lib

$(ODIR)/turbojpeg.obj: turbojpegipp.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

JPEGLINK = $(IPPLINK)

endif
endif

$(EDIR)/turbojpeg.dll $(LDIR)/turbojpeg.lib: $(ODIR)/turbojpeg.obj $(JPEGDEP)
	$(LINK) -dll -out:$(EDIR)/turbojpeg.dll -implib:$(LDIR)/turbojpeg.lib $< $(JPEGLINK)

$(EDIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(LDIR)/hputil.lib $(LDIR)/turbojpeg.lib
	$(LINK) $< -OUT:$@ $(HPULIB) turbojpeg.lib

$(EDIR)/jpegut.exe: $(ODIR)/jpegut.obj $(LDIR)/turbojpeg.lib
	$(LINK) $< -OUT:$@ turbojpeg.lib

$(EDIR)/24to32.exe: $(ODIR)/24to32.obj
	$(LINK) $< -OUT:$@

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libturbojpeg.so \
          $(EDIR)/jpgtest \
          $(EDIR)/jpegut \
          $(EDIR)/24to32

OBJS = $(ODIR)/turbojpeg.o \
       $(ODIR)/jpgtest.o \
       $(ODIR)/jpegut.o \
       $(ODIR)/24to32.o

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

$(ODIR)/turbojpeg.o: turbojpegp.c
	$(CC) $(CFLAGS) -I$(PEGDIR)/include -c $< -o $@

JPEGLINK = -L$(PEGDIR)/lib -lpicl20

else

ifeq ($(JPEGLIB), libjpeg)

$(ODIR)/turbojpeg.o: turbojpegl.c
	$(CC) -Ijpeg-6b/ $(CFLAGS) -c $< -o $@

JPEGLINK = $(LDIR)/libjpeg.a
JPEGDEP = $(LDIR)/libjpeg.a

else

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

$(ODIR)/turbojpeg.o: turbojpegipp.c
	$(CC) -I$(IPPDIR)/include $(CFLAGS) -c $< -o $@

JPEGLINK = $(IPPLINK)

endif
endif

$(LDIR)/libturbojpeg.so: $(ODIR)/turbojpeg.o $(JPEGDEP)
	$(CC) $(LDFLAGS) -shared $< -o $@ $(JPEGLINK)

$(EDIR)/jpgtest: $(ODIR)/jpgtest.o $(LDIR)/libhputil.a $(LDIR)/libturbojpeg.so
	$(CC) $(LDFLAGS) $< -o $@ -lhputil -lturbojpeg

$(EDIR)/jpegut: $(ODIR)/jpegut.o $(LDIR)/libturbojpeg.so
	$(CC) $(LDFLAGS) $< -o $@ -lturbojpeg

$(EDIR)/24to32: $(ODIR)/24to32.o
	$(CC) $(LDFLAGS) $< -o $@

##########################################################################
endif
##########################################################################
