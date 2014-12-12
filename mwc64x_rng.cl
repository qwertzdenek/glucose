/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
mwc64x_rng.cl

Part of MWC64X by David Thomas, dt10@imperial.ac.uk
Original: http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html

Modified by Zdeněk Janeček for different range setup
*/

#ifndef dt10_mwc64x_rng_cl
#define dt10_mwc64x_rng_cl

//! Represents the state of a particular generator
typedef struct{ uint x; uint c; } mwc64x_state_t;

enum{ MWC64X_A = 4294883355U };
enum{ MWC64X_M = 18446383549859758079UL };

// Pre: a<M, b<M
// Post: r=(a+b) mod M
ulong MWC_AddMod64(ulong a, ulong b, ulong M)
{
	ulong v=a+b;
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
ulong MWC_MulMod64(ulong a, ulong b, ulong M)
{
	ulong r=0;
	while(a!=0){
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
ulong MWC_PowMod64(ulong a, ulong e, ulong M)
{
	ulong sqr=a, acc=1;
	while(e!=0){
		if(e&1)
			acc=MWC_MulMod64(acc,sqr,M);
		sqr=MWC_MulMod64(sqr,sqr,M);
		e=e>>1;
	}
	return acc;
}

uint2 MWC_SeedImpl(ulong A, ulong M, uint range, ulong seed)
{
	ulong dist=get_global_id(0)*range;
	ulong m=MWC_PowMod64(A, dist, M);

	ulong x=MWC_MulMod64(seed, m, M);
	return (uint2)((uint)(x/A), (uint)(x%A));
}

void MWC64X_SeedStreams(mwc64x_state_t *s, uint range, ulong seed)
{
	uint2 tmp=MWC_SeedImpl(MWC64X_A, MWC64X_M, range, seed);
	s->x=tmp.x;
	s->c=tmp.y;
}

void MWC64X_Step(mwc64x_state_t *s)
{
	uint X=s->x, C=s->c;

	uint Xn=MWC64X_A*X+C;
	uint carry=(uint)(Xn<C);				// The (Xn<C) will be zero or one for scalar
	uint Cn=mad_hi(MWC64X_A,X,carry);

	s->x=Xn;
	s->c=Cn;
}

//! Return a 32-bit integer in the range [0..range)
uint MWC64X_NextUint(mwc64x_state_t *s, uint range)
{
	uint res=(s->x ^ s->c) % range;
	MWC64X_Step(s);
	return res;
}

#endif
