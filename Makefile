CC = cc
CPPFLAGS = -O3
INCLUDES = 

LIBS = -lm
OBJS = yuvfmt.o 
TARGET = yuvfmt

all : $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
%.o : %.c *.h
	$(CC) $(MACROS) $(INCLUDES) $(CPPFLAGS) -c $<
clean : 
	rm -f $(OBJS) $(TARGET)