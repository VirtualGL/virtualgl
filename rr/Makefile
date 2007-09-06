include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

GLFOBJS = $(ODIR)/rrglframe.obj
ifneq ($(XWIN32), yes)
GLFOBJS := $(GLFOBJS) $(ODIR)/xdk-sym.obj
endif

COBJS = $(ODIR)/rrcwin.obj \
        $(ODIR)/rrdisplayserver.obj \
        $(ODIR)/rrxclient.obj \
        $(ODIR)/rrframe.obj \
        $(GLFOBJS)

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

RRDEP = $(LDIR)/rrutil.lib $(LDIR)/fbx-x11.lib $(LDIR)/rrsocket.lib
ifneq ($(JPEGLIB), ipp)
RRDEP := $(RRDEP) $(LDIR)/turbojpeg.lib
endif
RRLIB = rrutil.lib $(FBXX11LIB) $(FAKEGLXLIB) turbojpeg.lib $(SOCKLIB)

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


$(EDIR)/vglclient.exe: $(COBJS) $(RRDEP)
	$(LINK) $(LDFLAGS) $(COBJS) -out:$@ $(RRLIB) advapi32.lib user32.lib

# Unit tests

$(EDIR)/rrframeut.exe: $(ODIR)/rrframeut.obj $(RRDEP)
	$(LINK) $(LDFLAGS) $< -out:$@ $(RRLIB) $(GLFOBJS) \
		$(ODIR)/rrframe.obj

$(EDIR)/rrblitterut.exe: $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj $(RRDEP)
	$(LINK) $(LDFLAGS) $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj -out:$@ $(RRLIB) \
		$(ODIR)/rrframe.obj

$(EDIR)/rrdisplayclientut.exe: $(ODIR)/rrdisplayclientut.obj \
	$(ODIR)/rrdisplayclient.obj $(RRDEP)
	$(LINK) $(LDFLAGS) $(ODIR)/rrdisplayclientut.obj $(ODIR)/rrdisplayclient.obj \
		-out:$@ $(RRLIB)

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
        $(ODIR)/rrsunray.o

TOBJS = $(ODIR)/rrblitterut.o \
        $(ODIR)/rrdisplayclientut.o \
        $(ODIR)/rrdisplayserverut.o \
        $(ODIR)/rrfakerut.o \
        $(ODIR)/rrframeut.o \
        $(ODIR)/dlfakerut.o

OBJS = $(COBJS) $(FOBJS) $(TOBJS) $(ODIR)/dlfaker.o $(ODIR)/vglconfig.o

UNITTESTS = $(EDIR)/rrblitterut \
            $(EDIR)/rrdisplayclientut \
            $(EDIR)/rrdisplayserverut \
            $(EDIR)/rrframeut
ifneq ($(platform), cygwin)
UNITTESTS := $(UNITTESTS) $(EDIR)/rrfakerut $(EDIR)/dlfakerut
endif

TARGETS = $(EDIR)/vglclient \
          $(UNITTESTS)

ifneq ($(platform), cygwin)
VGLCONFIG = $(EDIR)/vglconfig
ifeq ($(subplatform), 64)
VGLCONFIG = $(EDIR)/vglconfig64
endif
TARGETS :=  $(LDIR)/librrfaker.$(SHEXT) \
	$(LDIR)/libdlfaker.$(SHEXT) $(VGLCONFIG) $(TARGETS)
endif

RRDEP = $(LDIR)/librrutil.a $(LDIR)/libfbx.a $(LDIR)/librrsocket.a
ifneq ($(JPEGLIB), ipp)
ifeq ($(platform), cygwin)
RRDEP := $(RRDEP) $(LDIR)/turbojpeg.lib
else
RRDEP := $(RRDEP) $(LDIR)/libturbojpeg.$(SHEXT)
endif
endif
RRLIB = -lrrutil $(FBXLIB) -lturbojpeg $(SOCKLIB) -lm $(MLIB)

TARGETS := $(TARGETS) $(EDIR)/vglrun


all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d) $(ODIR)/faker-mapfile.E

-include $(OBJS:.o=.d)

# Server

$(LDIR)/librrfaker.$(SHEXT): $(ODIR)/faker-mapfile.E $(FOBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(FOBJS) $(RRLIB) $(FAKERLINK) -ldl

$(LDIR)/libdlfaker.$(SHEXT): $(ODIR)/dlfaker.o
	$(CC) $(LDFLAGS) $(SHFLAG) -o $@ $< -ldl

$(EDIR)/vglrun: rrlaunch
	$(INSTALL) -m 755 $< $@

$(VGLCONFIG): $(ODIR)/vglconfig.o $(LDIR)/libfltk_static.a
	$(CXX) $(LDFLAGS) -o $@ $< -lX11 -lfltk_static -lpthread -lrrutil

# Client

ifeq ($(platform), solaris)
$(EDIR)/vglclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB) $(LAZY) -lGL $(NOLAZY) -ldl
else
$(EDIR)/vglclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB) $(LAZY) -lGL $(NOLAZY)
endif

# Unit tests

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB) -lGL

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< $(ZNOW) -lGL -lGLU $(ZNOW) -lX11 -ldl -lpthread

$(EDIR)/dlfakerut: $(ODIR)/dlfakerut.o
	$(CC) $(LDFLAGS) -o $@ $< -lX11 -ldl

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
