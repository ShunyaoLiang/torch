CFLAGS = -Wall -I lib/ -I lib/termkey -I src/
LDFLAGS = -lm -lrt -lcurses

objects = $(addprefix src/, torch.o world.o player.o draw.o demo.o raycast.o random.o $(addprefix \
          ui/, term.o sdl.o)) $(addprefix lib/termkey/, driver-csi.o driver-ti.o termkey.o)

all: $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -O3 -march=native -D NDEBUG $(objects) -o bin/torch

debug: src/torch.h lib/list.h
	$(CC) $(CFLAGS) $(LDFLAGS) -D DEBUG -g src/*.c src/ui/term.c lib/termkey/*.c -o bin/debug

clean:
	rm -f $(objects)
