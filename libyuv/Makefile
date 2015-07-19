LIBSIMDIRS = ../libsim
LIBYUVDIRS = 
INCLUDES = $(LIBSIMDIRS):$(LIBYUVDIRS)
LIBSIM = $(LIBSIMDIRS)/libsim.a

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
LIBYUVSRCS = yuvdef.c yuvcvt_b8tile.c yuvcvt_b10.c 
LIBYUVSRCS += yuvcvt.c yuvfmt.c yuvcmp.c
LIBYUVOBJS = $(LIBYUVSRCS:%.c=$(TMPDIR)/%.o)
LIBYUV = libyuv.a


.PHONY: all
all : libsim $(LIBYUV)

.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(LIBYUV)
	cd $(LIBSIMDIRS); make clean

$(LIBYUV): $(LIBYUVOBJS) | libsim
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^

$(LIBYUVOBJS): $(LIBSIM) *.h Makefile
$(LIBYUVOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBYUVDIRS) -l$(LIBSIM) -o $@ $<

.PHONY: libsim
libsim $(LIBSIM):
	cd $(LIBSIMDIRS); make

$(TMPDIR):
	mkdir $(TMPDIR)
