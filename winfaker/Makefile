include ../Makerules

TARGETS = $(EDIR)/wglspheres.exe \
          $(EDIR)/faker.dll

FOBJS = $(ODIR)/faker.obj \
        $(ODIR)/pbwin.obj \
        $(ODIR)/rrblitter.obj

OBJS = $(FOBJS) \
       $(ODIR)/wglspheres.obj

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

CFLAGS := $(subst -I$(TOPDIR)/$(platform)$(subplatform)/include/xdk ,,$(CFLAGS))
CXXFLAGS := $(subst -I$(TOPDIR)/$(platform)$(subplatform)/include/xdk ,,$(CXXFLAGS))

HDRS := $(wildcard ../include/*.h) $(wildcard *.h)
$(OBJS): $(HDRS)

$(EDIR)/wglspheres.exe: $(ODIR)/wglspheres.obj
	$(LINK) $(LDFLAGS) $< -out:$@ opengl32.lib glu32.lib user32.lib gdi32.lib

$(EDIR)/faker.dll: $(FOBJS)
	$(LINK) $(LDFLAGS) -dll $(FOBJS) -out:$@ detours.lib detoured.lib \
		opengl32.lib rrutil.lib $(FBXLIB)
