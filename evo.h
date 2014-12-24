/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.h
*/

#ifndef EVO_H_INCLUDED
#define EVO_H_INCLUDED

#include "structures.h"

void evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type);
void evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type);
void evolution_opencl(int num_values, mvalue_ptr *values, bounds bconf, int metric_type);

void cl_cleanup();

#endif // EVO_H_INCLUDED
