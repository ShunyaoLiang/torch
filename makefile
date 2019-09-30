all: torch.c torch.h entity.h floor.h list.h
	gcc torch.c -ltickit
