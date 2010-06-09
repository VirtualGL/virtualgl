include ../Makerules
include ../jpeg/Makerules.tjpeg

##########################################################################
ifeq ($(platform), windows)
##########################################################################

GLFOBJS = $(ODIR)/rrglframe.obj $(ODIR)/xdk-sym.obj

COBJS = $(ODIR)/rrcwin.obj \
        $(ODIR)/rrdisplayserver.obj \
        $(ODIR)/vglclient.obj \
        $(ODIR)/rrframe.obj \
        $(GLFOBJS)

TOBJS = $(ODIR)/rrdisplayclient.obj \
        $(ODIR)/rrdisplayclientut.obj \
        $(ODIR)/rrframeut.obj \
        $(ODIR)/fakerconfig.obj

OBJS = $(COBJS) $(TOBJS)

UNITTESTS = $(EDIR)/rrdisplayclientut.exe \
            $(EDIR)/rrframeut.exe

TARGETS = $(EDIR)/vglclient.exe \
          $(EDIR)/vglconnect.bat \
          $(UNITTESTS)

RRDEP = $(LDIR)/rrutil.lib $(LDIR)/fbx-x11.lib $(LDIR)/rrsocket.lib $(JPEGDEP)
RRLIB = rrutil.lib $(FBXX11LIB) $(FAKEGLXLIB) $(JPEGLINK) $(SOCKLIB)

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


$(EDIR)/vglclient.exe: $(COBJS) $(RRDEP)
	$(LINK) $(LDFLAGS) $(COBJS) -out:$@ $(RRLIB) advapi32.lib user32.lib \
		psapi.lib

$(EDIR)/vglconnect.bat: vglconnect.bat
	cp $< $@

# Unit tests

$(EDIR)/rrframeut.exe: $(ODIR)/rrframeut.obj $(RRDEP)
	$(LINK) $(LDFLAGS) $< -out:$@ $(RRLIB) $(GLFOBJS) \
		$(ODIR)/rrframe.obj

$(EDIR)/rrdisplayclientut.exe: $(ODIR)/rrdisplayclientut.obj \
	$(ODIR)/rrdisplayclient.obj $(ODIR)/fakerconfig.obj $(RRDEP)
	$(LINK) $(LDFLAGS) $(ODIR)/rrdisplayclientut.obj $(ODIR)/rrdisplayclient.obj \
		$(ODIR)/fakerconfig.obj -out:$@ $(RRLIB)

##########################################################################
else
##########################################################################

COBJS = $(ODIR)/rrcwin.o \
        $(ODIR)/rrdisplayserver.o \
        $(ODIR)/vglclient.o

FOBJS = $(ODIR)/faker.o \
        $(ODIR)/faker-sym.o \
        $(ODIR)/pbwin.o \
        $(ODIR)/glxvisual.o \
        $(ODIR)/rrblitter.o \
        $(ODIR)/rrdisplayclient.o \
        $(ODIR)/rrplugin.o \
        $(ODIR)/fakerconfig.o

TOBJS = $(ODIR)/rrblitterut.o \
        $(ODIR)/rrdisplayclientut.o \
        $(ODIR)/rrdisplayserverut.o \
        $(ODIR)/rrfakerut.o \
        $(ODIR)/rrframeut.o \
        $(ODIR)/dlfakerut.o \
        $(ODIR)/libGLdlfakerut.o \
        $(ODIR)/testplugin.o \
        $(ODIR)/testplugin2.o

ifeq ($(USEXV), yes)
 FOBJS := $(FOBJS) $(ODIR)/rrxvtrans.o
endif

OBJS = $(COBJS) $(FOBJS) $(TOBJS) $(ODIR)/dlfaker.o $(ODIR)/gefaker.o $(ODIR)/vglconfig.o

UNITTESTS = $(EDIR)/rrblitterut \
            $(EDIR)/rrdisplayclientut \
            $(EDIR)/rrdisplayserverut \
            $(EDIR)/rrframeut
ifneq ($(platform), cygwin)
 ifneq ($(platform), osx)
  ifneq ($(platform), osxx86)
   UNITTESTS := $(UNITTESTS) $(EDIR)/rrfakerut $(EDIR)/dlfakerut \
                $(LDIR)/libGLdlfakerut.$(SHEXT)
  endif
 endif
endif

TARGETS = $(EDIR)/vglclient \
          $(EDIR)/vglconnect \
          $(UNITTESTS)

ifneq ($(platform), cygwin)
 ifneq ($(platform), osx)
  ifneq ($(platform), osxx86)
   TARGETS :=  $(LDIR)/librrfaker.$(SHEXT) $(LDIR)/libvgltrans_test.$(SHEXT) \
     $(LDIR)/libvgltrans_test2.$(SHEXT) $(LDIR)/libdlfaker.$(SHEXT) \
     $(LDIR)/libgefaker.$(SHEXT) $(EDIR)/vglconfig $(EDIR)/vglrun \
     $(EDIR)/vgllogin $(TARGETS)
  endif
 endif
endif

RRDEP = $(LDIR)/librrutil.a $(LDIR)/libfbx.a $(LDIR)/librrsocket.a $(JPEGDEP)
RRLIB = -lrrutil $(FBXLIB) $(JPEGLINK) $(SOCKLIB) -lm

ifeq ($(USEXV), yes)
 RRDEP := $(RRDEP) $(LDIR)/libfbxv.a
 RRLIB := $(RRLIB) -lXv -lfbxv
endif


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d) $(ODIR)/faker-mapfile.E

-include $(OBJS:.o=.d)

# Server

$(LDIR)/librrfaker.$(SHEXT): $(ODIR)/faker-mapfile.E $(FOBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(FOBJS) $(RRLIB) $(FAKERLINK) \
		$(MAPFLAG)$(ODIR)/faker-mapfile.E $(LIBDL)

$(LDIR)/libdlfaker.$(SHEXT): $(ODIR)/dlfaker.o
	$(CC) $(LDFLAGS) $(SHFLAG) -o $@ $< $(LIBDL)

$(LDIR)/libgefaker.$(SHEXT): $(ODIR)/gefaker.o
	$(CC) $(LDFLAGS) $(SHFLAG) -o $@ $< $(LIBDL)

$(EDIR)/vglrun: vglrun
	$(INSTALL) -m 755 $< $@

$(EDIR)/vgllogin: vgllogin
	$(INSTALL) -m 755 $< $@

VGLCONFIGLINK = -lX11 -lfltk_static -lpthread -lrrutil
ifeq ($(USEXV), yes)
VGLCONFIGLINK := $(VGLCONFIGLINK) -lXv
endif
$(EDIR)/vglconfig: $(ODIR)/vglconfig.o $(ODIR)/fakerconfig.o \
	$(LDIR)/libfltk_static.a
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/vglconfig.o $(ODIR)/fakerconfig.o \
		$(VGLCONFIGLINK)

# Client

ifeq ($(platform), solaris)
$(EDIR)/vglclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB) $(LAZY) $(GLLIB) $(NOLAZY) $(LIBDL)
else
$(EDIR)/vglclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB) $(LAZY) $(GLLIB) $(NOLAZY) $(OSXGLFIX)
endif

$(EDIR)/vglconnect: vglconnect
	$(INSTALL) -m 755 $< $@

# Unit tests

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB) $(GLLIB) $(OSXGLFIX)

LXMU=
ifeq ($(platform), solaris)
 LXMU=-lXmu
endif

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< $(ZNOW) $(GLLIB) -lGLU $(ZNOW) -lX11 $(LIBDL) -lpthread $(LXMU)

$(EDIR)/dlfakerut: $(ODIR)/dlfakerut.o
	$(CC) $(LDFLAGS) -o $@ $< -lX11 $(LIBDL)

$(LDIR)/libGLdlfakerut.$(SHEXT): $(ODIR)/libGLdlfakerut.o
	$(CC) $(LDFLAGS) $(SHFLAG) -o $@ $<

$(EDIR)/rrblitterut: $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o \
	$(ODIR)/fakerconfig.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o \
		$(ODIR)/fakerconfig.o $(RRLIB)

$(EDIR)/rrdisplayclientut: $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
	$(ODIR)/fakerconfig.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
		$(ODIR)/fakerconfig.o $(RRLIB)

$(EDIR)/rrdisplayserverut: $(ODIR)/rrdisplayserverut.o $(ODIR)/rrdisplayclient.o \
	$(ODIR)/fakerconfig.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayserverut.o $(ODIR)/rrdisplayclient.o \
		$(ODIR)/fakerconfig.o $(RRLIB)

$(LDIR)/libvgltrans_test.$(SHEXT): $(ODIR)/testplugin.o \
	$(ODIR)/rrdisplayclient.o $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(ODIR)/testplugin.o \
		$(ODIR)/rrdisplayclient.o $(RRLIB)

$(LDIR)/libvgltrans_test2.$(SHEXT): $(ODIR)/testplugin2.o \
	$(ODIR)/rrblitter.o $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(ODIR)/testplugin2.o \
		$(ODIR)/rrblitter.o $(RRLIB)

##########################################################################
endif
##########################################################################
