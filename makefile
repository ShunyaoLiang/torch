all: torch.c torch.h
	gcc torch.c -ltickit
