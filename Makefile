CC = cc
CPPFLAGS = -O3

INCLUDES = 

LIBS = -lm
OBJS = yuvdef.o yuvcvt_b8tile.o yuvcvt_b10.o yuvcvt.o yuvcmp.o yuvopt.o yuvmain.o
TARGET = yuv

all : $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(INCLUDES) $(CPPFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
%.o : %.c *.h
	$(CC) $(INCLUDES) $(CPPFLAGS) -c $<
clean : 
	rm -f $(OBJS) $(TARGET)
