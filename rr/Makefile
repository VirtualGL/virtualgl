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

OBJS = $(SOBJS) $(ODIR)/rrxclient.obj $(ODIR)/rrlistenerut.obj $(ODIR)/rrframeut.obj

TARGETS = $(LDIR)/rr.lib \
          $(EDIR)/rrxclient.exe \
          $(EDIR)/rrlistenerut.exe \
          $(EDIR)/rrframeut.exe

RRDEP = $(LDIR)/rr.lib $(LDIR)/hputil.lib $(LDIR)/hpsecnet.lib $(LDIR)/fbx-xdk.lib $(LDIR)/hpjpeg.lib
RRLIB = $(LDIR)/rr.lib $(HPULIB) $(HPSECNETLIB) $(LDIR)/fbx-xdk.lib $(XDKLIB) \
	$(LDIR)/hpjpeg.lib

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard faker-*.c*) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(LDIR)/rr.lib: $(SOBJS)
	$(AR) -OUT:$@ $^

$(EDIR)/rrxclient.exe: $(ODIR)/rrxclient.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB)

$(EDIR)/rrlistenerut.exe: $(ODIR)/rrlistenerut.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB)

$(EDIR)/rrframeut.exe: $(ODIR)/rrframeut.obj $(RRDEP)
	$(LINK) $< -OUT:$@ $(RRLIB)

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

OBJS = $(SOBJS) $(FOBJS) $(ODIR)/rrxclient.o $(ODIR)/rrlistenerut.o $(ODIR)/rrframeut.o $(ODIR)/rrfakerut.o

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
          $(EDIR)/$(RRLAUNCH)

RRDEP = $(LDIR)/librr.a $(LDIR)/libhputil.a $(LDIR)/libhpsecnet.a $(LDIR)/libfbx.a $(LDIR)/libhpjpeg.so
RRLIB = -lrr $(HPULIB) $(HPSECNETLIB) $(FBXLIB) -lhpjpeg

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

$(EDIR)/rrlistenerut: $(ODIR)/rrlistenerut.o $(ODIR)/rrlistener.o $(ODIR)/rrconn.o \
	$(LDIR)/libhputil.a $(LDIR)/libhpsecnet.a
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrlistenerut.o $(ODIR)/rrlistener.o $(ODIR)/rrconn.o \
		$(HPULIB) $(HPSECNETLIB)

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(LDIR)/libhputil.a $(LDIR)/libfbx.a $(LDIR)/libhpjpeg.so
	$(CXX) $(LDFLAGS) -o $@ $< $(HPULIB) $(FBXLIB) -lhpjpeg -lpthread

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< -lGL -lGLU -L/usr/X11R6/lib -lX11

##########################################################################
endif
##########################################################################
