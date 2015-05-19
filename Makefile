CC = cc
CPPFLAGS = -O3
INCLUDES = 

LIBS = -lm
OBJS = yuvdef.o 
OBJS += yuvcvt_b8tile.o yuvcvt_b10.o yuvcvt.o yuvfmt.o
OBJS += yuvcmp.o 
OBJS += sim_opt.o 
OBJS += yuvmain.o
TARGET = yuv

all : $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
%.o : %.c *.h
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) -c $<
clean : 
	rm -f $(OBJS) $(TARGET)
