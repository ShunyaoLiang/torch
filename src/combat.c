#include "torch.h"

int combat_do(struct combat *restrict c, struct combat *restrict target)
{
	target->hp--;
	return 0;
}
