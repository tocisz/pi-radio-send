CC=gcc
CFLAGS=-Wall
LFLAGS=-lwiringPi -lrt
DEPS =
OBJ = send.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

send: $(OBJ)
	gcc -o $@ $^ $(LFLAGS)
