include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

COBJS = $(ODIR)/rrcwin.obj \
        $(ODIR)/rrdisplayserver.obj \
        $(ODIR)/rrxclient.obj \
        $(ODIR)/rrframe.obj \
        $(ODIR)/rrglframe.obj
ifneq ($(XWIN32), yes)
COBJS := $(COBJS) $(ODIR)/xdk-sym.obj
endif

TOBJS = $(ODIR)/rrblitter.obj \
        $(ODIR)/rrblitterut.obj \
        $(ODIR)/rrdisplayclient.obj \
        $(ODIR)/rrdisplayclientut.obj \
        $(ODIR)/rrframeut.obj

OBJS = $(COBJS) $(TOBJS)

UNITTESTS = $(EDIR)/rrblitterut.exe \
            $(EDIR)/rrdisplayclientut.exe \
            $(EDIR)/rrframeut.exe

TARGETS = $(EDIR)/vglclient.exe \
          $(UNITTESTS)

RRDEP = $(LDIR)/rrutil.lib $(LDIR)/fbx-x11.lib $(LDIR)/turbojpeg.lib $(LDIR)/rrsocket.lib
RRLIB = -lrrutil $(FBXX11LIB) $(FAKEGLXLIB) -lturbojpeg $(SOCKLIB)

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


$(EDIR)/vglclient.exe: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o  $@ $(RRLIB)

# Unit tests

$(EDIR)/rrframeut.exe: $(ODIR)/rrframeut.obj $(RRDEP)
	$(CXX) $(LDFLAGS) $< -o $@ $(RRLIB) $(ODIR)/xdk-sym.obj $(ODIR)/rrglframe.obj \
		$(ODIR)/rrframe.obj

$(EDIR)/rrblitterut.exe: $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj $(RRDEP)
	$(CXX) $(LDFLAGS) $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj -o $@ $(RRLIB) \
		$(ODIR)/rrframe.obj

$(EDIR)/rrdisplayclientut.exe: $(ODIR)/rrdisplayclientut.obj \
	$(ODIR)/rrdisplayclient.obj $(RRDEP)
	$(CXX) $(LDFLAGS) $(ODIR)/rrdisplayclientut.obj $(ODIR)/rrdisplayclient.obj \
		-o $@ $(RRLIB)

##########################################################################
else
##########################################################################

COBJS = $(ODIR)/rrcwin.o \
        $(ODIR)/rrdisplayserver.o \
        $(ODIR)/rrxclient.o

FOBJS = $(ODIR)/faker.o \
        $(ODIR)/faker-sym.o \
        $(ODIR)/pbwin.o \
        $(ODIR)/glxvisual.o \
        $(ODIR)/rrblitter.o \
        $(ODIR)/rrdisplayclient.o \
        $(ODIR)/vglgui.o

TOBJS = $(ODIR)/rrblitterut.o \
        $(ODIR)/rrdisplayclientut.o \
        $(ODIR)/rrdisplayserverut.o \
        $(ODIR)/rrfakerut.o \
        $(ODIR)/rrframeut.o

ifeq ($(platform), solaris)
FOBJS := $(FOBJS) $(ODIR)/rrsunray.o
endif
ifeq ($(platform), solx86)
FOBJS := $(FOBJS) $(ODIR)/rrsunray.o
endif
ifeq ($(platform), linux)
FOBJS := $(FOBJS) $(ODIR)/rrsunray.o
endif

OBJS = $(COBJS) $(FOBJS) $(TOBJS) $(ODIR)/rrdccwrap.o

UNITTESTS = $(EDIR)/rrblitterut \
            $(EDIR)/rrdisplayclientut \
            $(EDIR)/rrdisplayserverut \
            $(EDIR)/rrframeut
ifneq ($(platform), cygwin)
UNITTESTS := $(UNITTESTS) $(EDIR)/rrfakerut
endif

TARGETS = $(EDIR)/vglclient \
          $(UNITTESTS)

ifneq ($(platform), cygwin)
TARGETS :=  $(LDIR)/librr.$(SHEXT) $(LDIR)/librrfaker.$(SHEXT) $(TARGETS)
endif

RRDEP = $(LDIR)/librrutil.a $(LDIR)/libfbx.a $(LDIR)/librrsocket.a
ifneq ($(platform), linux)
ifeq ($(platform), cygwin)
RRDEP := $(RRDEP) $(LDIR)/turbojpeg.lib
else
RRDEP := $(RRDEP) $(LDIR)/libturbojpeg.$(SHEXT)
endif
endif
RRLIB = -lrrutil $(FBXLIB) -lturbojpeg $(SOCKLIB)

TARGETS := $(TARGETS) $(EDIR)/vglrun


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

# Server

$(LDIR)/librrfaker.$(SHEXT): $(FOBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(FOBJS) $(RRLIB) $(FAKERLINK) -ldl -lXt -lXaw

$(LDIR)/librr.$(SHEXT): $(ODIR)/rrdccwrap.o $(ODIR)/rrblitter.o $(ODIR)/rrdisplayclient.o $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(ODIR)/rrdccwrap.o $(ODIR)/rrblitter.o $(ODIR)/rrdisplayclient.o $(RRLIB)

$(EDIR)/vglrun: rrlaunch
	$(INSTALL) -m 755 $< $@

# Client

LIBGL = -zlazyload -lGL -znolazyload
ifneq ($(platform), solaris)
ifneq ($(platform), solx86)
LIBGL = -lGL
endif
endif

$(EDIR)/vglclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB) $(LIBGL)

# Unit tests

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB) -lGL

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< -z now -lGL -lGLU -z now -lX11 -ldl -lpthread

$(EDIR)/rrblitterut: $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRLIB)

$(EDIR)/rrdisplayclientut: $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
	$(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
		$(RRLIB)

$(EDIR)/rrdisplayserverut: $(ODIR)/rrdisplayserverut.o $(ODIR)/rrdisplayclient.o \
	$(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayserverut.o $(ODIR)/rrdisplayclient.o \
		$(RRLIB)

##########################################################################
endif
##########################################################################
