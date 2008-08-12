include ../Makerules

TARGETS = $(EDIR)/wglreadtest.exe \
          $(EDIR)/wglspheres.exe \
          $(EDIR)/faker.dll

FOBJS = $(ODIR)/faker.obj \
        $(ODIR)/pbwin.obj \
        $(ODIR)/rrblitter.obj

OBJS = $(FOBJS) \
       $(ODIR)/wglreadtest.obj \
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

$(EDIR)/wglreadtest.exe: $(ODIR)/wglreadtest.obj
	$(LINK) $(LDFLAGS) $< -out:$@ opengl32.lib glu32.lib user32.lib gdi32.lib

$(EDIR)/faker.dll: $(FOBJS)
	$(LINK) $(LDFLAGS) -dll $(FOBJS) -out:$@ detours.lib detoured.lib \
		opengl32.lib rrutil.lib $(FBXLIB)

ifneq ($(DISTRO),)
WBLDDIR = $(TOPDIR)\\$(platform)$(subplatform)\\$(DISTRO)
else
WBLDDIR = $(TOPDIR)\\$(platform)$(subplatform)
endif

ifeq ($(DEBUG), yes)
WBLDDIR := $(WBLDDIR)\\dbg
endif

DETOURSDIR=$$%systemdrive%\\Program Files\\Microsoft Research\\Detours Express 2.1

dist: all
	$(RM) $(WBLDDIR)\$(APPNAME)-Server.exe
	makensis //DAPPNAME=$(APPNAME)-Server //DVERSION=$(VERSION) \
		//DBUILD=$(BUILD) //DBLDDIR=$(WBLDDIR) //DDETOURSDIR="$(DETOURSDIR)" winfaker.nsi || \
	makensis /DAPPNAME=$(APPNAME)-Server /DVERSION=$(VERSION) \
		/DBUILD=$(BUILD) /DBLDDIR=$(WBLDDIR) /DDETOURSDIR="$(DETOURSDIR)" winfaker.nsi  # Cygwin doesn't like the //
