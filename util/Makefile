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
          $(LDIR)/fbx-xdk.lib \
          $(EDIR)/fbxtest-xdk.exe

OBJS = $(ODIR)/rrlog.obj \
       $(ODIR)/genericQ.obj \
       $(ODIR)/rrsocket.obj \
       $(ODIR)/fbx.obj \
       $(ODIR)/fbxtest.obj \
       $(ODIR)/nettest.obj \
       $(ODIR)/threadtest.obj \
       $(ODIR)/fbx-xdk.obj \
       $(ODIR)/fbxtest-xdk.obj

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(LDIR)/rrutil.lib: $(ODIR)/rrlog.obj $(ODIR)/genericQ.obj
	$(AR) -OUT:$@ $^

$(ODIR)/fbx-xdk.obj: fbx.c
	$(CC) $(CFLAGS) $(XDKINC) -DXDK -UWIN32 -c $< -Fo$@

$(ODIR)/fbxtest-xdk.obj: fbxtest.cpp
	$(CXX) $(CFLAGS) $(XDKINC) -DXDK -UWIN32 -c $< -Fo$@

$(EDIR)/nettest.exe: $(ODIR)/nettest.obj $(LDIR)/rrsocket.lib
	$(LINK) $< -out:$@ $(SOCKLIB)

$(EDIR)/fbxtest-xdk.exe: $(ODIR)/fbxtest-xdk.obj $(LDIR)/fbx-xdk.lib
	$(LINK) $< -out:$@ $(FBXXDKLIB)

$(EDIR)/fbxtest.exe: $(ODIR)/fbxtest.obj $(LDIR)/fbx.lib
	$(LINK) $< -out:$@ $(FBXLIB)

$(EDIR)/threadtest.exe :$(ODIR)/threadtest.obj
	$(LINK) $< -out:$@

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/librrutil.a \
          $(LDIR)/librrsocket.a \
          $(LDIR)/libfbx.a \
          $(EDIR)/fbxtest \
          $(EDIR)/nettest \
          $(EDIR)/threadtest

OBJS = $(ODIR)/rrlog.o \
       $(ODIR)/genericQ.o \
       $(ODIR)/rrsocket.o \
       $(ODIR)/fbx.o \
       $(ODIR)/fbxtest.o \
       $(ODIR)/nettest.o \
       $(ODIR)/threadtest.o

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

-include $(OBJS:.o=.d)

$(LDIR)/librrutil.a: $(ODIR)/rrlog.o $(ODIR)/genericQ.o
	$(AR) $@ $^

$(EDIR)/nettest: $(ODIR)/nettest.o $(LDIR)/librrsocket.a
	$(CXX) $(LDFLAGS) $< -o $@ $(SOCKLIB)

$(EDIR)/fbxtest: $(ODIR)/fbxtest.o $(LDIR)/libfbx.a
	$(CXX) $(LDFLAGS) $< -o $@ $(FBXLIB) -lpthread

$(EDIR)/threadtest: $(ODIR)/threadtest.o
	$(CXX) $(LDFLAGS) $< -o $@ -lpthread

##########################################################################
endif
##########################################################################
