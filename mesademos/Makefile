include ../Makerules

##########################################################################
ifeq ($(platform), windows)
##########################################################################

all:

##########################################################################
else
##########################################################################

SRCS := $(wildcard *.c)

OBJS := $(addprefix $(ODIR)/,$(SRCS:.c=.o))

TARGETS = $(EDIR)/glthreads \
          $(EDIR)/glthreadsint \
          $(EDIR)/glxdemo \
          $(EDIR)/glxgears \
          $(EDIR)/glxheads \
          $(EDIR)/glxinfo \
          $(EDIR)/glxpixmap \
          $(EDIR)/manywin \
          $(EDIR)/offset \
          $(EDIR)/pbdemo \
          $(EDIR)/pbinfo \
          $(EDIR)/shape \
          $(EDIR)/testgl5 \
          $(EDIR)/wincopy \
          $(EDIR)/xfont

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

$(ODIR)/glthreads.o \
$(ODIR)/glthreadsint.o: $(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) -DPTHREADS -c -o $@ $<

$(EDIR)/glthreads \
$(EDIR)/glthreadsint: $(EDIR)/%: $(ODIR)/%.o
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lpthread -lm

$(EDIR)/glxinfo \
$(EDIR)/offset: $(EDIR)/%: $(ODIR)/%.o
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lGLU -lm

$(EDIR)/glxgears \
$(EDIR)/glxdemo \
$(EDIR)/glxheads \
$(EDIR)/glxpixmap \
$(EDIR)/manywin \
$(EDIR)/shape \
$(EDIR)/testgl5 \
$(EDIR)/wincopy \
$(EDIR)/xfont : $(EDIR)/%: $(ODIR)/%.o
	$(CC) $(LDFLAGS) -o $@ $< -lGL -lX11 -lXext -lm

$(EDIR)/pbdemo \
$(EDIR)/pbinfo: $(EDIR)/%: $(ODIR)/%.o $(ODIR)/pbutil.o
	$(CC) $(LDFLAGS) -o $@ $^ -lGL -lX11 -lm

##########################################################################
endif
##########################################################################
