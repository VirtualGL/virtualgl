include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(LDIR)/hputil.lib \
          $(LDIR)/hpsecnet.lib \
          $(LDIR)/fbx.lib \
          $(EDIR)/fbxtest.exe \
          $(EDIR)/nettest.exe \
          $(LDIR)/fbx-xdk.lib \
          $(EDIR)/fbxtest-xdk.exe

OBJS = $(ODIR)/hputil.obj \
       $(ODIR)/hpsecnet.obj \
       $(ODIR)/fbx.obj \
       $(ODIR)/fbxtest.obj \
       $(ODIR)/nettest.obj \
       $(ODIR)/fbx-xdk.obj \
       $(ODIR)/fbxtest-xdk.obj

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(ODIR)/fbx-xdk.obj: fbx.c
	$(CC) $(CFLAGS) $(XDKINC) -DXDK -UWIN32 -c $< -Fo$@

$(ODIR)/fbxtest-xdk.obj: fbxtest.cpp
	$(CXX) $(CFLAGS) $(XDKINC) -DXDK -UWIN32 -c $< -Fo$@

$(ODIR)/nettest.obj: nettest.c
	$(CC) $(CFLAGS) -DSECNET -c $< -Fo$@

$(EDIR)/nettest.exe: $(ODIR)/nettest.obj $(LDIR)/hputil.lib $(LDIR)/hpsecnet.lib
	$(LINK) $< -OUT:$@ $(HPULIB) $(HPSECNETLIB)

$(EDIR)/fbxtest-xdk.exe: $(ODIR)/fbxtest-xdk.obj $(LDIR)/fbx-xdk.lib $(LDIR)/hputil.lib
	$(LINK) $< -out:$@ fbx-xdk.lib $(XDKLIB) pthreadVC.lib $(HPULIB)

$(EDIR)/fbxtest.exe: $(ODIR)/fbxtest.obj $(LDIR)/fbx.lib $(LDIR)/hputil.lib
	$(LINK) $< -out:$@ $(FBXLIB) pthreadVC.lib $(HPULIB)

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libhputil.a \
          $(LDIR)/libhpsecnet.a \
          $(LDIR)/libfbx.a \
          $(EDIR)/fbxtest \
          $(EDIR)/nettest

OBJS = $(ODIR)/hputil.o \
       $(ODIR)/hpsecnet.o \
       $(ODIR)/fbx.o \
       $(ODIR)/fbxtest.o \
       $(ODIR)/nettest.o

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

-include $(OBJS:.o=.d)

$(ODIR)/nettest.o: nettest.c
	$(CC) $(CFLAGS) -DSECNET -c $< -o $@

$(EDIR)/nettest: $(ODIR)/nettest.o $(LDIR)/libhputil.a $(LDIR)/libhpsecnet.a
	$(CC) $(LDFLAGS) $< -o $@ -lhputil -lhpsecnet -lpthread -lssl

$(EDIR)/fbxtest: $(ODIR)/fbxtest.o $(LDIR)/libfbx.a $(LDIR)/libhputil.a
	$(CXX) $(LDFLAGS) $< -o $@ $(FBXLIB) -lpthread $(HPULIB)

##########################################################################
endif
##########################################################################
