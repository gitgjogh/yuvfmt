TOPDIR = .
LIBSIMDIRS = $(TOPDIR)/libsim
LIBYUVDIRS = $(TOPDIR)/libyuv
INCLUDES = $(LIBSIMDIRS):$(LIBYUVDIRS)
VPATH = $(TOPDIR):$(LIBSIMDIRS):$(LIBYUVDIRS)

CC = gcc
CFLAGS = -c -O3

LIBS = -lm

TMPDIR = mk.tmp
LIBSIMSRCS = sim_opt.c sim_utils.c
LIBSIMOBJS = $(LIBSIMSRCS:%.c=$(TMPDIR)/%.o)
LIBSIM = libsim.a

LIBYUVSRCS = yuvdef.c yuvcvt_b8tile.c yuvcvt_b10.c 
LIBYUVSRCS += yuvcvt.c yuvfmt.c yuvcmp.c
LIBSIMSRCS = sim_opt.c sim_utils.c
LIBYUVOBJS = $(LIBYUVSRCS:%.c=$(TMPDIR)/%.o)
LIBYUV = libyuv.a

BINSRCS = yuvmain.c 
BINOBJS = $(BINSRCS:%.c=$(TMPDIR)/%.o)
OUTBIN = yuv


.PHONY: all
all : $(LIBSIM) $(LIBYUV) $(OUTBIN) 


.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(LIBSIM)
	rm -rf $(LIBYUV)
	rm -rf $(OUTBIN)

$(LIBSIM): $(LIBSIMOBJS) 
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^
    
$(LIBYUV): $(LIBYUVOBJS)
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^
	
$(OUTBIN): $(BINOBJS) $(LIBSIM) $(LIBYUV)
	@echo; echo "[LD] linking ..."
	cc -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -o $@ $^

$(LIBSIMOBJS): libsim/*.h Makefile
$(LIBSIMOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -o $@ $<
    
$(LIBYUVOBJS): libsim/*.h libyuv/*.h Makefile
$(LIBYUVOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -o $@ $<

$(BINOBJS): libsim/*.h libyuv/*.h Makefile
$(BINOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -o $@ $<

$(TMPDIR):
	mkdir $(TMPDIR)
