CC = clang
CFLAGS = -Wall -O3
LDFLAGS = -ltermkey -lunibilium -Llib -ltickit -lm

objects = torch.o entity.o player.o floor.o draw.o main_win.o demo.o

all: $(objects) bin/libtickit.a
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o bin/torch

debug: torch.h list.h bin/libtickit.a
	$(CC) *.c -g $(CFLAGS) $(LDFLAGS) -o bin/debug

clean:
	rm $(objects)
