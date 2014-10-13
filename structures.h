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
    float p;
    float cg;
    float c;
    float pp;
    float cgp;
    float cp;
    float dt;
    float h;
    float k;
    float m;
    float n;
    float fitness;
} member;

typedef struct
{
    float pmin;
    float pmax;
    float cgmin;
    float cgmax;
    float cmin;
    float cmax;

    float ppmin;
    float ppmax;
    float cgpmin;
    float cgpmax;
    float cpmin;
    float cpmax;

    float dtmin;
    float dtmax;
    float hmin;
    float hmax;

    float kmin;
    float kmax;

    float mmin;
    float mmax;
    float nmin;
    float nmax;
} bounds;

#endif // STRUCTURES_H_INCLUDED
