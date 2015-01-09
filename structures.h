/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
structures.h
*/

#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

#define POPULATION_SIZE 30

#define METRIC_ABS 10
#define METRIC_SQ 11
#define METRIC_MAX 12

typedef struct
{
    float blood;
    float ist;
    float time;
} mvalue;

typedef struct
{
    mvalue *vals;
    int cvals;
} mvalue_ptr;

typedef struct
{
    float p;   // s0
    float cg;  // s1
    float c;   // s2
    float pp;  // s3
    float cgp; // s4
    float cp;  // s5
    float dt;  // s6
    float h;   // s7
    float k;   // s8
    float m;   // s9
    float n;   // sa
    float fitness; // sb
    float reserved[4];
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
