include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

TARGETS = $(LDIR)/hputil.lib \
          $(LDIR)/hpsecnet.lib \
          $(LDIR)/fbx.lib \
          $(EDIR)/nettest.exe \
          $(LDIR)/fbx-xdk.lib

OBJS = $(ODIR)/hputil.obj \
       $(ODIR)/hpsecnet.obj \
       $(ODIR)/fbx.obj \
       $(ODIR)/nettest.obj \
       $(ODIR)/fbx-xdk.obj

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard ../include/*.h)
$(OBJS): $(HDRS)

$(ODIR)/fbx-xdk.obj: fbx.c
	$(CC) $(CFLAGS) $(XDKINC) -DXDK -UWIN32 -c $< -Fo$@

$(ODIR)/nettest.obj: nettest.c
	$(CC) $(CFLAGS) -DSECNET -c $< -Fo$@

$(EDIR)/nettest.exe: $(ODIR)/nettest.obj $(LDIR)/hputil.lib $(LDIR)/hpsecnet.lib
	$(LINK) $< -OUT:$@ $(HPULIB) $(HPSECNETLIB)

##########################################################################
else
##########################################################################

TARGETS = $(LDIR)/libhputil.a \
          $(LDIR)/libhpsecnet.a \
          $(LDIR)/libfbx.a \
          $(EDIR)/nettest

OBJS = $(ODIR)/hputil.o \
       $(ODIR)/hpsecnet.o \
       $(ODIR)/fbx.o \
       $(ODIR)/nettest.o

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

-include $(OBJS:.o=.d)

$(ODIR)/nettest.o: nettest.c
	$(CC) $(CFLAGS) -DSECNET -c $< -o $@

$(EDIR)/nettest: $(ODIR)/nettest.o $(LDIR)/libhputil.a $(LDIR)/libhpsecnet.a
	$(CC) $(LDFLAGS) $< -o $@ -lhputil -lhpsecnet -lpthread -lssl

##########################################################################
endif
##########################################################################
