CC = cc
CPPFLAGS = -O3
MACROS = -D_XLOG_
INCLUDES = 

LIBS = -lm
OBJS = yuvdef.o 
OBJS += yuvcvt_b8tile.o yuvcvt_b10.o yuvcvt.o 
OBJS += yuvcmp.o 
OBJS += sim_log.o sim_opt.o 
OBJS += yuvmain.o
TARGET = yuv

all : $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
%.o : %.c *.h
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) -c $<
clean : 
	rm -f $(OBJS) $(TARGET)
