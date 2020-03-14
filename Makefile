CFLAGS = -Wall -I lib/ -I lib/termkey -I src/
LDFLAGS = -lm -lrt -lunibilium

objects = $(addprefix src/, torch.o world.o player.o draw.o demo.o color.o raycast.o random.o combat.o $(addprefix ui/, term.o sdl.o)) $(addprefix lib/termkey/, driver-csi.o driver-ti.o termkey.o)

all: $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o bin/torch -O3 -D NDEBUG

lib/termkey/driver-ti.o:
	$(CC) lib/termkey/driver-ti.c $(CFLAGS) -c -o lib/termkey/driver-ti.o -D HAVE_UNIBILIUM

debug: src/torch.h lib/list.h
	$(CC) src/*.c -g src/ui/term.c lib/termkey/*.c $(CFLAGS) $(LDFLAGS) -o bin/debug -D DEBUG -D HAVE_UNIBILIUM

clean:
	rm -f $(objects) bin/*
