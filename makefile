CC = clang
CFLAGS = -Wall
LDFLAGS = -ltickit -lm

objects = torch.o entity.o player.o floor.o draw.o main_win.o demo.o

all: $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS)

debug: torch.h list.h
	$(CC) *.c -g $(CFLAGS) $(LDFLAGS)

clean:
	rm $(objects)
