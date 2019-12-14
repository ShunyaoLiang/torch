CC = clang
CFLAGS = -Wall -O3 -I lib/
LDFLAGS = -ltickit -lm

objects = $(addprefix src/, torch.o entity.o player.o floor.o draw.o main_win.o demo.o)

all: $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o bin/torch

debug: src/torch.h lib/list.h
	$(CC) src/*.c -g $(CFLAGS) $(LDFLAGS) -o bin/debug

clean:
	rm -f $(objects)
