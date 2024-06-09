SRC := socket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)
CC = $(CROSS_COMPILE)gcc

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -pthread $(INCLUDES) $(OBJS) -o $(TARGET) ${LIBS} $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
