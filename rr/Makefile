include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

COBJS = $(ODIR)/rrcwin.obj \
        $(ODIR)/rrdisplayserver.obj \
        $(ODIR)/rrxclient.obj

TOBJS = $(ODIR)/rrblitter.obj \
        $(ODIR)/rrblitterut.obj \
        $(ODIR)/rrdisplayclient.obj \
        $(ODIR)/rrdisplayclientut.obj \
        $(ODIR)/rrframeut.obj

OBJS = $(COBJS) $(TOBJS)

UNITTESTS = $(EDIR)/rrblitterut.exe \
            $(EDIR)/rrdisplayclientut.exe \
            $(EDIR)/rrframeut.exe

TARGETS = $(EDIR)/rrxclient.exe \
          $(UNITTESTS)

RRDEP = $(LDIR)/rrutil.lib $(LDIR)/fbx-xdk.lib $(LDIR)/hpjpeg.lib $(LDIR)/rrsocket.lib
RRLIB = rrutil.lib $(FBXXDKLIB) hpjpeg.lib $(SOCKLIB)

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h) $(wildcard ../include/*.h)
$(OBJS): $(HDRS)


$(EDIR)/rrxclient.exe: $(COBJS) $(RRDEP)
	$(LINK) $(COBJS) -OUT:$@ $(RRLIB)

# Unit tests

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

COBJS = $(ODIR)/rrcwin.o \
        $(ODIR)/rrdisplayserver.o \
        $(ODIR)/rrxclient.o

FOBJS = $(ODIR)/faker.o \
        $(ODIR)/faker-sym.o \
        $(ODIR)/pbwin.o \
        $(ODIR)/glxvisual.o \
        $(ODIR)/rrblitter.o \
        $(ODIR)/rrdisplayclient.o

TOBJS = $(ODIR)/rrblitterut.o \
        $(ODIR)/rrdisplayclientut.o \
        $(ODIR)/rrfakerut.o \
        $(ODIR)/rrframeut.o

OBJS = $(COBJS) $(FOBJS) $(TOBJS)

UNITTESTS = $(EDIR)/rrblitterut \
            $(EDIR)/rrdisplayclientut \
            $(EDIR)/rrfakerut \
            $(EDIR)/rrframeut \

TARGETS = $(LDIR)/librrfaker.so \
          $(EDIR)/rrxclient \
          $(UNITTESTS)


RRDEP = $(LDIR)/librrutil.a $(LDIR)/libfbx.a $(LDIR)/libhpjpeg.so $(LDIR)/librrsocket.a
RRLIB = -lrrutil $(FBXLIB) -lhpjpeg $(SOCKLIB)
RRLAUNCH = rrlaunch
ifeq ($(subplatform), 64)
RRLAUNCH = rrlaunch64
endif

TARGETS := $(TARGETS) $(EDIR)/$(RRLAUNCH)

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

# Server

$(LDIR)/librrfaker.so: $(FOBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(SHFLAG) -o $@ $(FOBJS) $(RRLIB)

$(EDIR)/$(RRLAUNCH): $(RRLAUNCH)
	cp $(RRLAUNCH) $@; chmod 755 $@

# Client

$(EDIR)/rrxclient: $(COBJS) $(RRDEP)
	$(CXX) $(LDFLAGS) $(COBJS) -o $@ $(RRLIB)

# Unit tests

$(EDIR)/rrframeut: $(ODIR)/rrframeut.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $< $(RRLIB)

$(EDIR)/rrfakerut: $(ODIR)/rrfakerut.o
	$(CXX) $(LDFLAGS) -o $@ $< -lGL -lGLU -lX11

$(EDIR)/rrblitterut: $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrblitterut.o $(ODIR)/rrblitter.o $(RRLIB)

$(EDIR)/rrdisplayclientut: $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
	$(RRDEP)
	$(CXX) $(LDFLAGS) -o $@ $(ODIR)/rrdisplayclientut.o $(ODIR)/rrdisplayclient.o \
		$(RRLIB)

##########################################################################
endif
##########################################################################
