CC = clang
CFLAGS = -Wall
LDFLAGS = -ltickit -lm

objects = torch.o entity.o player.o floor.o draw.o main_win.o demo.o

all: torch entity player floor draw main_win demo
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS)

torch: torch.c torch.h list.h
	$(CC) torch.c -c $(CFLAGS)

entity: torch.h
	$(CC) entity.c -c $(CFLAGS)

player: torch.h list.h
	$(CC) player.c -c $(CFLAGS)

floor: torch.h list.h
	$(CC) floor.c -c $(CFLAGS)

draw: torch.h
	$(CC) draw.c -c $(CFLAGS)

main_win: torch.h
	$(CC) main_win.c -c $(CFLAGS)

demo: torch.h
	$(CC) demo.c -c $(CFLAGS)

debug: torch.h list.h
	$(CC) *.c -g $(CFLAGS) $(LDFLAGS)

clean:
	rm $(objects)
