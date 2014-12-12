#ifndef MWC64X_RNG_H_INCLUDED
#define MWC64X_RNG_H_INCLUDED

void MWC64X_Seed(uint64_t *state, uint32_t range, uint64_t seed);
uint32_t MWC64X_Next(uint64_t *s, uint32_t range);

#endif // MWC64X_RNG_H_INCLUDED
