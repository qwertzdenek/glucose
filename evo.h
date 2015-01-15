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

#define F 0.45 // mutation constant
#define CR 0.4 // threshold

#define PENALTY 10000.0f

extern int num_generations; // generation count

int evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result, double *time_used);
int evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result, double *time_used);
int evolution_opencl(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result, double *time_used);

void cl_cleanup();

#endif // EVO_H_INCLUDED
