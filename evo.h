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

typedef float (*get_metric_func)(float left, float right);

float metric_abs(float left, float right);
float metric_square(float left, float right);

void evolution_serial(mvalue_ptr *values, int size, bounds bconf, get_metric_func mfun);
void evolution_pthread(mvalue_ptr *values, int size, bounds bconf, get_metric_func mfun);

#endif // EVO_H_INCLUDED
