include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

OBJS = $(ODIR)/tcbench.obj \
       $(ODIR)/imgdiff.obj

TARGETS = $(EDIR)/tcbench.exe \
          $(EDIR)/imgdiff.exe

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

$(EDIR)/tcbench.exe: $(ODIR)/tcbench.obj $(LDIR)/fbx.lib
	$(LINK) $^ -OUT:$@ $(FBXLIB) user32.lib
	
$(EDIR)/imgdiff.exe: $(ODIR)/imgdiff.obj
	$(LINK) $^ -OUT:$@

##########################################################################
else
##########################################################################

OBJS = $(ODIR)/tcbench.o \
			 $(ODIR)/dsimple.o \
			 $(ODIR)/cpustat.o \
			 $(ODIR)/imgdiff.o

TARGETS = $(EDIR)/tcbench \
					$(EDIR)/cpustat \
					$(EDIR)/imgdiff

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

$(EDIR)/tcbench: $(ODIR)/tcbench.o $(ODIR)/dsimple.o $(LDIR)/libfbx.a
	$(CXX) $(LDFLAGS) $(ODIR)/tcbench.o $(ODIR)/dsimple.o -o $@ $(FBXLIB)

$(EDIR)/cpustat: $(ODIR)/cpustat.o
	$(CC) $(LDFLAGS) $< -o $@

$(EDIR)/imgdiff: $(ODIR)/imgdiff.o
	$(CC) $(LDFLAGS) $< -o $@

##########################################################################
endif
##########################################################################
