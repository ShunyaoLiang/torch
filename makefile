all: torch.c torch.h entity.h floor.h list.h blend
	gcc torch.c blend.o -ltickit

blend: blend.c blend.h floor.h
	gcc blend.c -c
