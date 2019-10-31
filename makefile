CC = clang
CFLAGS = -Wall
LDFLAGS = -ltickit -lm

all: torch
	$(CC) torch.o $(CFLAGS) $(LDFLAGS)

torch: torch.c torch.h list.h
	$(CC) torch.c -c $(CFLAGS)

debug: torch.c torch.h list.h
	$(CC) torch.c -g $(CFLAGS) $(LDFLAGS)
