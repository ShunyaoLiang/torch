CFLAGS = -Wall -I lib/ -I lib/termkey -I src/ -D HAVE_UNIBILIUM
LDFLAGS = -lm -lrt -lunibilium

objects = $(addprefix src/, torch.o entity.o player.o floor.o draw.o input.o demo.o color.o raycast.o random.o $(addprefix ui/, term.o sdl.o)) $(addprefix lib/termkey/, driver-csi.o driver-ti.o termkey.o)

all: $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o bin/torch

debug: src/torch.h lib/list.h
	$(CC) src/*.c -g src/ui/term.c lib/termkey/*.c $(CFLAGS) $(LDFLAGS) -o bin/debug -D DEBUG

clean:
	rm -f $(objects)
