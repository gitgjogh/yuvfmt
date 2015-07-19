LIBSIMDIRS = .
INCLUDES = $(LIBSIMDIRS)
VPATH = $(LIBSIMDIRS)

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
LIBSIMSRCS = sim_opt.c sim_utils.c
LIBSIMOBJS = $(LIBSIMSRCS:%.c=$(TMPDIR)/%.o)
LIBSIM = libsim.a

.PHONY: all
all : $(LIBSIM)

.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(LIBSIM)

$(LIBSIM): $(LIBSIMOBJS)
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^

$(LIBSIMOBJS): *.h Makefile
$(LIBSIMOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -o $@ $<

$(TMPDIR):
	mkdir -p $@