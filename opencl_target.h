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

int  cl_init(int num_values, mvalue_ptr *values, int num_members, member *members, int metric_type);
void cl_compute_fitness(long seed);
void cl_read_result(member *res);
void cl_cleanup();

#endif // OPENCL_TARGET_H_INCLUDED
