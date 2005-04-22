include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

all:

##########################################################################
else
##########################################################################

OBJS = $(ODIR)/rrglxgears.o

TARGETS = $(EDIR)/rrglxgears

ifeq ($(USEGLP), yes)
OBJS := $(OBJS) $(ODIR)/rrglpgears.o

TARGETS := $(TARGETS) $(EDIR)/rrglpgears
endif

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

$(ODIR)/rrglxgears.o: rrglxgears.c ../rr/rr.h
	$(CC) $(CFLAGS) -c -o $@ $< -I../rr

$(EDIR)/rrglxgears: $(ODIR)/rrglxgears.o $(LDIR)/librr.$(SHEXT)
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lXext -lm -lrr -lhpjpeg

$(ODIR)/rrglpgears.o: rrglpgears.c ../rr/rr.h
	$(CC) $(CFLAGS) -c -o $@ $< -I../rr

$(EDIR)/rrglpgears: $(ODIR)/rrglpgears.o $(LDIR)/librr.$(SHEXT)
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lXext -lm -lrr -lhpjpeg

##########################################################################
endif
##########################################################################
