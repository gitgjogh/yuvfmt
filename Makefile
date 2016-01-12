LIBSIMDIRS = zbase/libsim
LIBYUVDIRS = libyuv
INCLUDES = $(LIBSIMDIRS):$(LIBYUVDIRS)
LIBSIM = $(LIBSIMDIRS)/libsim.a
LIBYUV = $(LIBYUVDIRS)/libyuv.a

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
BINSRCS = yuvmain.c 
BINOBJS = $(BINSRCS:%.c=$(TMPDIR)/%.o)
OUTBIN = yuv


.PHONY: all
all : libyuv $(OUTBIN) 


.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(OUTBIN)
	cd $(LIBYUVDIRS); make clean
	
$(OUTBIN): $(BINOBJS) $(LIBSIM) $(LIBYUV) | libyuv
	@echo; echo "[LD] linking ..."
	cc -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -o $@ $^ 

$(BINOBJS): $(LIBYUV) Makefile
$(BINOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -l$(LIBSIM) -l$(LIBYUV) -o $@ $<

.PHONY: libyuv
libyuv $(LIBYUV):
	cd $(LIBYUVDIRS); make

$(TMPDIR):
	mkdir $(TMPDIR)
