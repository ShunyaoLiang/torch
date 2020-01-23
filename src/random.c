#include <time.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#define DO_ONCE(thing) do { \
	static bool done = false; \
	if (!done) { \
		thing; \
		done = true; \
	} \
} while (0); \

static unsigned int xorshift(unsigned int state);

int rand(void)
{
	static unsigned int state;
	DO_ONCE(state = time(NULL));

	return (state = xorshift(state)) % INT_MAX;
}

static unsigned int xorshift(unsigned int state)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	
	return state;
}
