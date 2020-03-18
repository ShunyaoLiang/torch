CFLAGS = -Wall -I lib/ -I lib/termkey -I src/
LDFLAGS = -lm -lrt -lunibilium

objects = $(addprefix src/, torch.o world.o player.o draw.o demo.o color.o raycast.o random.o combat.o $(addprefix ui/, term.o sdl.o)) $(addprefix lib/termkey/, driver-csi.o driver-ti.o termkey.o)

all: $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -O3 -march=native -D NDEBUG $(objects) -o bin/torch

lib/termkey/driver-ti.o:
	$(CC) $(CFLAGS) -D HAVE_UNIBILIUM lib/termkey/driver-ti.c -c -o lib/termkey/driver-ti.o

debug: src/torch.h lib/list.h
	$(CC) $(CFLAGS) $(LDFLAGS) -D DEBUG -D HAVE_UNIBILIUM -g src/*.c src/ui/term.c lib/termkey/*.c -o bin/debug

clean:
	rm -f $(objects)
