/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
structures.h
*/

#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

typedef struct
{
    float blood;
    float ist;
    double time;
} mvalue;

typedef struct
{
    mvalue *vals;
    int cvals;
} mvalue_ptr;

typedef struct
{
    signed char pmin;
    signed char pmax;
    float cgmin;
    float cgmax;
    signed char cmin;
    signed char cmax;

    signed char ppmin;
    signed char ppmax;
    signed char cgpmin;
    signed char cfpmax;
    signed char cpmin;
    signed char cpmax;

    float dtmin;
    float dtmax;
    float hmin;
    float hmax;

    signed char kmin;
    signed char kmax;

    signed char mmin;
    signed char mmax;
    signed char nmin;
    signed char nmax;
} conf;

#endif // STRUCTURES_H_INCLUDED
