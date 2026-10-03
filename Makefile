CC = gcc
CFLAGS = -O3 -pthread -Wall
TARGET = udp_stress

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
