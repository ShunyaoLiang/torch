CC = clang
CFLAGS = -Wall -I lib/ -I lib/termkey -I src/ -D HAVE_UNIBILIUM
LDFLAGS = -lm -lunibilium

objects = $(addprefix src/, torch.o entity.o player.o floor.o draw.o main_win.o demo.o $(addprefix ui/, term.o sdl.o)) $(addprefix lib/termkey/, driver-csi.o driver-ti.o termkey.o)

all: $(objects)
	$(CC) $(objects) $(CFLAGS) $(LDFLAGS) -o bin/torch

debug: src/torch.h lib/list.h
	$(CC) src/*.c -g src/ui/term.c lib/termkey/*.c $(CFLAGS) $(LDFLAGS) -o bin/debug

clean:
	rm -f $(objects)
