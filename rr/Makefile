include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

SOBJS = $(ODIR)/rrconn.obj \
        $(ODIR)/rrlistener.obj \
        $(ODIR)/rrcwin.obj \
        $(ODIR)/rrdisplayserver.obj \
        $(ODIR)/rrdscwrap.obj \
        $(ODIR)/genericQ.obj

OBJS = $(SOBJS) $(ODIR)/rrxclient.obj $(ODIR)/rrblitter.obj $(ODIR)/rrblitterut.obj $(ODIR)/rrdisplayclient.obj $(ODIR)/rrdisplayclientut.obj $(ODIR)/rrlistenerut.obj $(ODIR)/rrframeut.obj

TARGETS = $(LDIR)/rr.lib \
          $(EDIR)/rrxclient.exe \
          $(EDIR)/rrlistenerut.exe \
          $(EDIR)/rrframeut.exe \
          $(EDIR)/rrblitterut.exe \
          $(EDIR)/rrdisplayclientut.exe

RRDEP = $(LDIR)/rr.lib $(LDIR)/hputil.lib $(LDIR)/hpsecnet.lib $(LDIR)/fbx-xdk.lib $(LDIR)/turbojpeg.lib
RRLIB = $(LDIR)/rr.lib $(HPULIB) $(HPSECNETLIB) $(LDIR)/fbx-xdk.lib $(XDKLIB) \
	$(LDIR)/turbojpeg.lib

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard faker-*.c*) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(LDIR)/rr.lib: $(SOBJS)
	$(AR) -OUT:$@ $^

$(EDIR)/rrxclient.exe: $(ODIR)/rrxclient.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB) advapi32.lib

$(EDIR)/rrlistenerut.exe: $(ODIR)/rrlistenerut.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB)

$(EDIR)/rrframeut.exe: $(ODIR)/rrframeut.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB)

$(EDIR)/rrblitterut.exe: $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj $(RRDEP)
	$(LINK) $(ODIR)/rrblitterut.obj $(ODIR)/rrblitter.obj -OUT:$@ $(RRLIB)

$(EDIR)/rrdisplayclientut.exe: $(ODIR)/rrdisplayclientut.obj \
	$(ODIR)/rrdisplayclient.obj $(RRDEP)
	$(LINK) $(ODIR)/rrdisplayclientut.obj $(ODIR)/rrdisplayclient.obj \
		-OUT:$@ $(RRLIB)

##########################################################################
else
##########################################################################

SOBJS = $(ODIR)/rrconn.o \
        $(ODIR)/rrlistener.o \
        $(ODIR)/rrcwin.o \
        $(ODIR)/rrdisplayserver.o \
        $(ODIR)/rrdisplayclient.o \
        $(ODIR)/rrdscwrap.o \
        $(ODIR)/rrdccwrap.o \
        $(ODIR)/genericQ.o

FOBJS = $(ODIR)/faker.o \
        $(ODIR)/faker-sym.o \
        $(ODIR)/pbwin.o \
        $(ODIR)/glxvisual.o \
        $(ODIR)/rrblitter.o

OBJS = $(SOBJS) $(FOBJS) $(ODIR)/rrxclient.o $(ODIR)/rrblitterut.o $(ODIR)/rrdisplayclientut.o $(ODIR)/rrlistenerut.o $(ODIR)/rrframeut.o $(ODIR)/rrfakerut.o

RRLAUNCH = rrlaunch
ifeq ($(subplatform), 64)
RRLAUNCH = rrlaunch64
endif

TARGETS = $(LDIR)/librr.a \
          $(LDIR)/librrfaker.so \
          $(EDIR)/rrxclient \
          $(EDIR)/rrlistenerut \
          $(EDIR)/rrframeut \
          $(EDIR)/rrfakerut \
          $(EDIR)/rrblitterut \
          $(EDIR)/rrdisplayclientut \
          $(EDIR)/$(RRLAUNCH)

RRDEP = $(LDIR)/librr.a $(LDIR)/libhputil.a $(LDIR)/libhpsecnet.a $(LDIR)/libfbx.a $(LDIR)/libturbojpeg.so
RRLIB = -lrr $(HPULIB) $(HPSECNETLIB) $(FBXLIB) -lturbojpeg

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

# Server

$(LDIR)/librr.a: $(SOBJS)
	$(AR) $@ $^

$(LDIR)/librrfaker.so: $(FOBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) -shared -o $@ $(FOBJS) $(RRLIB)

$(EDIR)/$(RRLAUNCH): $(RRLAUNCH)
	install -m 755 $(RRLAUNCH) $@

# Client

$(EDIR)/rrxclient: $(ODIR)/rrxclient.o $(RRDEP)
	$(CXX) $(LDFLAGS) $< -o $@ $(RRLIB)

# Unit tests

$(EDIR)/rrlistenerut: $(ODIR)/rrlistenerut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB)

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB)

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< -lGL -lGLU -L/usr/X11R6/lib -lX11 -lpthread

$(EDIR)/rrblitterut: $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRLIB)

$(EDIR)/rrdisplayclientut: $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
	$(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
		$(RRLIB)

##########################################################################
endif
##########################################################################
