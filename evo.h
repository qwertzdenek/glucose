/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.h
*/

#ifndef EVO_H_INCLUDED
#define EVO_H_INCLUDED

#define EVO_ERROR -1
#define EVO_OK 0

#include "structures.h"

#define GENERATION_COUNT 500
#define F 0.45 // mutation constant
#define CR 0.4 // threshold

#define PENALTY 10000.0f

int evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result);
int evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result);
int evolution_opencl(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result);

void cl_cleanup();

#endif // EVO_H_INCLUDED
