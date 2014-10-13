#ifndef EVO_H_INCLUDED
#define EVO_H_INCLUDED

#include "structures.h"

#define POPULATION_SIZE 40

void evolution(mvalue_ptr *db_values, int db_size, bounds bconf, member members[]);

#endif // EVO_H_INCLUDED
