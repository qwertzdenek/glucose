/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
opencl_target.h
*/

#ifndef OPENCL_TARGET_H_INCLUDED
#define OPENCL_TARGET_H_INCLUDED

#define OPENCL_ERROR -1
#define OPENCL_OK 0

#include "structures.h"

int init_opencl(int num_values, mvalue_ptr *values, int size_members, int metric_type);
void cl_compute_fitness(member *members);

#endif // OPENCL_TARGET_H_INCLUDED
