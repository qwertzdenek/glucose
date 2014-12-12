/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
mwc64x_rng.c

Part of MWC64X by David Thomas, dt10@imperial.ac.uk
Original: http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html

Modified by Zdeněk Janeček for different range setup
*/

#include <stdint.h>

const uint64_t MWC64X_A = 4294883355UL;
const uint64_t MWC64X_M = 18446383549859758079UL;

// Pre: a<M, b<M
// Post: r=(a+b) mod M
uint64_t MWC_AddMod64(uint64_t a, uint64_t b, uint64_t M)
{
    uint64_t v=a+b;
    if( (v>=M) || (v<a) )
        v=v-M;
    return v;
}

// Pre: a<M,b<M
// Post: r=(a*b) mod M
// This could be done more efficently, but it is portable, and should
// be easy to understand. It can be replaced with any of the better
// modular multiplication algorithms (for example if you know you have
// double precision available or something).
uint64_t MWC_MulMod64(uint64_t a, uint64_t b, uint64_t M)
{
    uint64_t r=0;
    while(a!=0)
    {
        if(a&1)
            r=MWC_AddMod64(r,b,M);
        b=MWC_AddMod64(b,b,M);
        a=a>>1;
    }
    return r;
}

// Pre: a<M, e>=0
// Post: r=(a^b) mod M
// This takes at most ~64^2 modular additions, so probably about 2^15 or so instructions on
// most architectures
uint64_t MWC_PowMod64(uint64_t a, uint64_t e, uint64_t M)
{
	uint64_t sqr=a, acc=1;
	while(e!=0){
		if(e&1)
			acc=MWC_MulMod64(acc,sqr,M);
		sqr=MWC_MulMod64(sqr,sqr,M);
		e=e>>1;
	}
	return acc;
}

uint64_t MWC_SeedImpl(uint64_t A, uint64_t M, uint32_t range, uint64_t seed)
{
	uint64_t m=MWC_PowMod64(A, range, M);
	uint64_t x=MWC_MulMod64(seed, m, M);

    uint64_t res = x/A;
    res |= (x%A) << 32;

	return res;
}

void MWC64X_Seed(uint64_t *state, uint32_t range, uint64_t seed)
{
	*state = MWC_SeedImpl(MWC64X_A, MWC64X_M, range, seed);
}

// Return a 32-bit integer in the range [0..range)
uint32_t MWC64X_Next(uint64_t *s, uint32_t range)
{
    uint32_t x = (*s) & 0xFFFFFFFF;
    uint32_t c = (*s) >> 32;

    *s = (uint64_t) 4294883355U * x + c;
    return (x^c) % range;
}
