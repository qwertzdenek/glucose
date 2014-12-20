/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.h
*/

#ifndef EVO_H_INCLUDED
#define EVO_H_INCLUDED

#include "structures.h"

#define POPULATION_SIZE 30

#define METRIC_ABS 10
#define METRIC_SQ 11
#define METRIC_MAX 12

void evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type);
void evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type);

#endif // EVO_H_INCLUDED
