ONAME := obj/libjpeg

TOPDIR = ../..
include $(TOPDIR)/Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(LDIR)/libjpeg.lib \
          $(EDIR)/cjpeg.exe \
          $(EDIR)/djpeg.exe \
          $(EDIR)/jpegtran.exe \
          $(EDIR)/rdjpgcom.exe \
          $(EDIR)/wrjpgcom.exe

LOBJS = $(ODIR)/jcapimin.obj \
        $(ODIR)/jcapistd.obj \
        $(ODIR)/jccoefct.obj \
        $(ODIR)/jccolor.obj \
        $(ODIR)/jcdctmgr.obj \
        $(ODIR)/jchuff.obj \
        $(ODIR)/jcinit.obj \
        $(ODIR)/jcmainct.obj \
        $(ODIR)/jcmarker.obj \
        $(ODIR)/jcmaster.obj \
        $(ODIR)/jcomapi.obj \
        $(ODIR)/jcparam.obj \
        $(ODIR)/jcphuff.obj \
        $(ODIR)/jcprepct.obj \
        $(ODIR)/jcsample.obj \
        $(ODIR)/jctrans.obj \
        $(ODIR)/jdapimin.obj \
        $(ODIR)/jdapistd.obj \
        $(ODIR)/jdatadst.obj \
        $(ODIR)/jdatasrc.obj \
        $(ODIR)/jdcoefct.obj \
        $(ODIR)/jdcolor.obj \
        $(ODIR)/jddctmgr.obj \
        $(ODIR)/jdhuff.obj \
        $(ODIR)/jdinput.obj \
        $(ODIR)/jdmainct.obj \
        $(ODIR)/jdmarker.obj \
        $(ODIR)/jdmaster.obj \
        $(ODIR)/jdmerge.obj \
        $(ODIR)/jdphuff.obj \
        $(ODIR)/jdpostct.obj \
        $(ODIR)/jdsample.obj \
        $(ODIR)/jdtrans.obj \
        $(ODIR)/jerror.obj \
        $(ODIR)/jfdctflt.obj \
        $(ODIR)/jfdctfst.obj \
        $(ODIR)/jfdctint.obj \
        $(ODIR)/jidctflt.obj \
        $(ODIR)/jidctfst.obj \
        $(ODIR)/jidctint.obj \
        $(ODIR)/jidctred.obj \
        $(ODIR)/jquant1.obj \
        $(ODIR)/jquant2.obj \
        $(ODIR)/jutils.obj \
        $(ODIR)/jmemmgr.obj \
        $(ODIR)/jmemnobs.obj

OBJS := $(LOBJS) \
        $(ODIR)/cdjpeg.obj \
        $(ODIR)/cjpeg.obj \
        $(ODIR)/djpeg.obj \
        $(ODIR)/jpegtran.obj \
        $(ODIR)/rdbmp.obj \
        $(ODIR)/rdcolmap.obj \
        $(ODIR)/rdgif.obj \
        $(ODIR)/rdjpgcom.obj \
        $(ODIR)/rdppm.obj \
        $(ODIR)/rdtarga.obj \
        $(ODIR)/rdswitch.obj \
        $(ODIR)/rdtarga.obj \
        $(ODIR)/transupp.obj \
        $(ODIR)/wrbmp.obj \
        $(ODIR)/wrgif.obj \
        $(ODIR)/wrppm.obj \
        $(ODIR)/wrtarga.obj \
        $(ODIR)/wrjpgcom.obj


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h)
$(OBJS): $(HDRS)


$(LDIR)/libjpeg.lib: $(LOBJS)
	echo TARGETS = $(TARGETS)
	$(AR) -OUT:$@ $^

$(EDIR)/cjpeg.exe: $(ODIR)/cdjpeg.obj $(ODIR)/cjpeg.obj $(ODIR)/rdbmp.obj \
	$(ODIR)/rdgif.obj $(ODIR)/rdppm.obj $(ODIR)/rdswitch.obj \
	$(ODIR)/rdtarga.obj $(LDIR)/libjpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(EDIR)/djpeg.exe: $(ODIR)/cdjpeg.obj $(ODIR)/djpeg.obj $(ODIR)/rdcolmap.obj \
	$(ODIR)/rdswitch.obj $(ODIR)/wrbmp.obj $(ODIR)/wrgif.obj $(ODIR)/wrppm.obj \
	$(ODIR)/wrtarga.obj $(LDIR)/libjpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(EDIR)/jpegtran.exe: $(ODIR)/cdjpeg.obj $(ODIR)/jpegtran.obj \
	$(ODIR)/rdswitch.obj $(ODIR)/transupp.obj $(LDIR)/libjpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(EDIR)/rdjpgcom.exe: $(ODIR)/rdjpgcom.obj $(LDIR)/libjpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(EDIR)/wrjpgcom.exe: $(ODIR)/wrjpgcom.obj $(LDIR)/libjpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libjpeg.a \
          $(EDIR)/cjpeg \
          $(EDIR)/djpeg \
          $(EDIR)/jpegtran \
          $(EDIR)/rdjpgcom \
          $(EDIR)/wrjpgcom

LOBJS = $(ODIR)/jcapimin.o \
        $(ODIR)/jcapistd.o \
        $(ODIR)/jccoefct.o \
        $(ODIR)/jccolor.o \
        $(ODIR)/jcdctmgr.o \
        $(ODIR)/jchuff.o \
        $(ODIR)/jcinit.o \
        $(ODIR)/jcmainct.o \
        $(ODIR)/jcmarker.o \
        $(ODIR)/jcmaster.o \
        $(ODIR)/jcomapi.o \
        $(ODIR)/jcparam.o \
        $(ODIR)/jcphuff.o \
        $(ODIR)/jcprepct.o \
        $(ODIR)/jcsample.o \
        $(ODIR)/jctrans.o \
        $(ODIR)/jdapimin.o \
        $(ODIR)/jdapistd.o \
        $(ODIR)/jdatadst.o \
        $(ODIR)/jdatasrc.o \
        $(ODIR)/jdcoefct.o \
        $(ODIR)/jdcolor.o \
        $(ODIR)/jddctmgr.o \
        $(ODIR)/jdhuff.o \
        $(ODIR)/jdinput.o \
        $(ODIR)/jdmainct.o \
        $(ODIR)/jdmarker.o \
        $(ODIR)/jdmaster.o \
        $(ODIR)/jdmerge.o \
        $(ODIR)/jdphuff.o \
        $(ODIR)/jdpostct.o \
        $(ODIR)/jdsample.o \
        $(ODIR)/jdtrans.o \
        $(ODIR)/jerror.o \
        $(ODIR)/jfdctflt.o \
        $(ODIR)/jfdctfst.o \
        $(ODIR)/jfdctint.o \
        $(ODIR)/jidctflt.o \
        $(ODIR)/jidctfst.o \
        $(ODIR)/jidctint.o \
        $(ODIR)/jidctred.o \
        $(ODIR)/jquant1.o \
        $(ODIR)/jquant2.o \
        $(ODIR)/jutils.o \
        $(ODIR)/jmemmgr.o \
        $(ODIR)/jmemnobs.o

OBJS := $(LOBJS) \
        $(ODIR)/cdjpeg.o \
        $(ODIR)/cjpeg.o \
        $(ODIR)/djpeg.o \
        $(ODIR)/jpegtran.o \
        $(ODIR)/rdbmp.o \
        $(ODIR)/rdcolmap.o \
        $(ODIR)/rdgif.o \
        $(ODIR)/rdjpgcom.o \
        $(ODIR)/rdppm.o \
        $(ODIR)/rdtarga.o \
        $(ODIR)/rdswitch.o \
        $(ODIR)/rdtarga.o \
        $(ODIR)/transupp.o \
        $(ODIR)/wrbmp.o \
        $(ODIR)/wrgif.o \
        $(ODIR)/wrppm.o \
        $(ODIR)/wrtarga.o \
        $(ODIR)/wrjpgcom.o


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

$(LDIR)/libjpeg.a: $(LOBJS)
	$(AR) $@ $^

$(EDIR)/cjpeg: $(ODIR)/cdjpeg.o $(ODIR)/cjpeg.o $(ODIR)/rdbmp.o \
	$(ODIR)/rdgif.o $(ODIR)/rdppm.o $(ODIR)/rdswitch.o $(ODIR)/rdtarga.o \
	$(LDIR)/libjpeg.a
	$(CC) $(LDFLAGS) $^ -o $@

$(EDIR)/djpeg: $(ODIR)/cdjpeg.o $(ODIR)/djpeg.o $(ODIR)/rdcolmap.o \
	$(ODIR)/rdswitch.o $(ODIR)/wrbmp.o $(ODIR)/wrgif.o $(ODIR)/wrppm.o \
	$(ODIR)/wrtarga.o $(LDIR)/libjpeg.a
	$(CC) $(LDFLAGS) $^ -o $@

$(EDIR)/jpegtran: $(ODIR)/cdjpeg.o $(ODIR)/jpegtran.o $(ODIR)/rdswitch.o \
	$(ODIR)/transupp.o $(LDIR)/libjpeg.a
	$(CC) $(LDFLAGS) $^ -o $@

$(EDIR)/rdjpgcom: $(ODIR)/rdjpgcom.o $(LDIR)/libjpeg.a
	$(CC) $(LDFLAGS) $^ -o $@

$(EDIR)/wrjpgcom: $(ODIR)/wrjpgcom.o $(LDIR)/libjpeg.a
	$(CC) $(LDFLAGS) $^ -o $@

##########################################################################
endif
##########################################################################
