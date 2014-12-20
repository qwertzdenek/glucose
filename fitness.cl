/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
OpenCL evolution
*/

typedef struct
{
    float blood;
    float ist;
    float time;
} mvalue;

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

__kernel void equation_solve (__global const mvalue* seg_vals,
    __global const int* seg_lenghts, __local const member m, __global float* seg_vals_res, const char metric_type)
{
   const int idx = get_global_id(0);

   if (idx < 2 && idx >= seg_lenghts[idx] - 2)
      return;


}

__kernel void fitness_solve (__global const float* seg_vals_res,
    __global const int* seg_lenghts, __global const float* res, const char metric_type)
{
   const int idx = get_global_id(0);

   if (idx < 2 && idx >= seg_lenghts[idx] - 2)
      return;


}
