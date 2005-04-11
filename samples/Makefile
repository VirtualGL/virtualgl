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

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

$(ODIR)/rrglxgears.o: rrglxgears.c
	$(CC) $(CFLAGS) -c -o $@ $< -I../rr

$(EDIR)/rrglxgears: $(ODIR)/rrglxgears.o $(LDIR)/librr.$(SHEXT)
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lXext -lm -lrr

##########################################################################
endif
##########################################################################
