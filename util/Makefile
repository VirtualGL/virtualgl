include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(LDIR)/rrutil.lib \
          $(LDIR)/rrsocket.lib \
          $(LDIR)/fbx.lib \
          $(EDIR)/fbxtest.exe \
          $(EDIR)/nettest.exe \
          $(EDIR)/threadtest.exe \
          $(LDIR)/fbx-x11.lib \
          $(EDIR)/fbxtest-x11.exe \
          $(EDIR)/glreadtest.exe

ifeq ($(VNC), yes)
TARGETS = $(LDIR)/rrutil.lib \
          $(LDIR)/fbx.lib \
          $(EDIR)/fbxtest.exe
endif

OBJS = $(ODIR)/bmp.obj \
       $(ODIR)/rrlog.obj \
       $(ODIR)/genericQ.obj \
       $(ODIR)/rrsocket.obj \
       $(ODIR)/fbx.obj \
       $(ODIR)/fbxtest.obj \
       $(ODIR)/nettest.obj \
       $(ODIR)/threadtest.obj \
       $(ODIR)/fbx-x11.obj \
       $(ODIR)/fbxtest-x11.obj \
       $(ODIR)/glreadtest.obj \

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(LDIR)/rrutil.lib: $(ODIR)/rrlog.obj $(ODIR)/genericQ.obj $(ODIR)/bmp.obj
	$(AR) -OUT:$@ $^

$(ODIR)/fbx-x11.obj: fbx.c
	$(CC) $(CFLAGS) -DFBXX11 -c $< -Fo$@

$(ODIR)/fbxtest-x11.obj: fbxtest.cpp
	$(CXX) $(CXXFLAGS) -DFBXX11 -c $< -Fo$@

$(EDIR)/nettest.exe: $(ODIR)/nettest.obj $(LDIR)/rrsocket.lib
	$(LINK) $(LDFLAGS) $< -out:$@ $(SOCKLIB)

$(EDIR)/fbxtest-x11.exe: $(ODIR)/fbxtest-x11.obj $(LDIR)/fbx-x11.lib
	$(LINK) $(LDFLAGS) $< -out:$@ $(FBXX11LIB)

$(EDIR)/glreadtest.exe: $(ODIR)/glreadtest.obj
	$(LINK) $(LDFLAGS) $< -out:$@ $(GLXLIB) $(X11LIB) $(GLULIB)

$(EDIR)/fbxtest.exe: $(ODIR)/fbxtest.obj $(LDIR)/fbx.lib
	$(LINK) $(LDFLAGS) $< -out:$@ $(FBXLIB)

$(EDIR)/threadtest.exe :$(ODIR)/threadtest.obj
	$(LINK) $(LDFLAGS) $< -out:$@

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/librrutil.a \
          $(LDIR)/librrsocket.a \
          $(LDIR)/libfbx.a \
          $(EDIR)/fbxtest \
          $(EDIR)/nettest \
          $(EDIR)/threadtest \
          $(EDIR)/glreadtest

OBJS = $(ODIR)/bmp.o \
       $(ODIR)/rrlog.o \
       $(ODIR)/genericQ.o \
       $(ODIR)/rrsocket.o \
       $(ODIR)/fbx.o \
       $(ODIR)/fbxtest.o \
       $(ODIR)/nettest.o \
       $(ODIR)/threadtest.o \
       $(ODIR)/glreadtest.o

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

-include $(OBJS:.o=.d)

$(LDIR)/librrutil.a: $(ODIR)/rrlog.o $(ODIR)/genericQ.o $(ODIR)/bmp.o
	$(AR) $@ $^

ifeq ($(platform), solaris)
KSTAT = -lkstat
endif
ifeq ($(platform), solx86)
KSTAT = -lkstat
endif
$(EDIR)/nettest: $(ODIR)/nettest.o $(LDIR)/librrsocket.a
	$(CXX) $(LDFLAGS) $< -o $@ $(SOCKLIB) $(KSTAT)

$(EDIR)/fbxtest: $(ODIR)/fbxtest.o $(LDIR)/libfbx.a
	$(CXX) $(LDFLAGS) $< -o $@ $(FBXLIB) -lpthread

$(EDIR)/threadtest: $(ODIR)/threadtest.o
	$(CXX) $(LDFLAGS) $< -o $@ -lpthread

$(EDIR)/glreadtest: $(ODIR)/glreadtest.o
	$(CXX) $(LDFLAGS) $< -o $@ -lGL -lGLU -lX11

##########################################################################
endif
##########################################################################
