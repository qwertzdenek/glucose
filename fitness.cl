/*
The MIT License (MIT)
Copyright (c) 2014 Zden?k Jane?ek

** Glucose project**
OpenCL evolution
*/

#define METRIC_ABS 10
#define METRIC_SQ 11
#define METRIC_MAX 12

#define PENALTY 10000.0f

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

__kernel void solve_equation (const int num_seg_vals, __global const mvalue* seg_vals, __global const int* seg_lenghts,
                              const int num_members, __global const member *members,
                              __global float* seg_vals_res, const char metric_type)
{
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    float a, b; // ist vals
    float ta, tb, tc; // interpolated times
    float result;
    int s, tmpj;
    mvalue act;
    member m;

    const int idx = get_global_id(0); // time
    const int idy = get_global_id(1); // segment
    const int idz = get_global_id(2); // member
    const int seg = idy * get_global_size(0);
    const int len = seg_lenghts[idy];
    const int res_prefix = idz * num_seg_vals * get_global_size(0);

    if (idy >= num_seg_vals)
        return;

    if (idx < 2)
        return;

    if (idx >= len - 2)
        return;

    if (idz >= num_members)
        return;

    act = seg_vals[seg + idx];
    m = members[idz];

    ta = seg_vals[seg + idx - 1].time;
    tc = act.time - m.h; // i(t - h)

    if (tc < seg_vals[seg].time || tc > seg_vals[seg + len - 1].time)
    {
        seg_vals_res[res_prefix + seg + idx] = PENALTY;
        return;
    }

    tmpj = idx - 1;
    while (tc <= ta && --tmpj >= 0)
        ta = seg_vals[seg + tmpj].time;

    tmpj = tmpj + 1;

    tb = seg_vals[seg + tmpj].time;
    b = seg_vals[seg + tmpj].ist;
    a = seg_vals[seg + tmpj - 1].ist;

    itmh = (tc - ta) / (tb - ta) * (b - a) + a; // i(t - h)

    phi = (act.ist - itmh) / m.h;

    // others
    psi = act.blood * (act.blood - act.ist);
    I = m.p * act.blood + m.cg * psi + m.c;
    theta = phi * (m.pp * act.blood + m.cgp * psi + m.cp);

    left = I + theta;

    // ** RIGHT
    ta = act.time;
    tc = act.time + m.dt + m.k * phi;

    if (tc < seg_vals[seg].time || tc > seg_vals[seg + len - 1].time)
    {
        seg_vals_res[res_prefix + seg + idx] = PENALTY;
        return;
    }

    s = (int) sign(tc - ta);

    switch (s)
    {
    case 1:
        tmpj = idx + 1;
        tb = seg_vals[seg + tmpj].time;
        while (tc > tb && ++tmpj < len)
            tb = seg_vals[seg + tmpj].time;

        tmpj = tmpj - 1;
        ta = seg_vals[seg + tmpj].time;
        break;
    case -1:
        tmpj = idx - 1;
        ta = seg_vals[seg + tmpj].time;
        while (tc < ta && --tmpj >= 0)
            ta = seg_vals[seg + tmpj].time;

        tb = seg_vals[seg + tmpj + 1].time;
        break;
    } // tmpj -> a, tmpj + 1 -> b

    a = seg_vals[seg + tmpj].ist;
    b = seg_vals[seg + tmpj + 1].ist;

    ipm = (tc - ta) / (tb - ta) * (b - a) + a;

    tb = seg_vals[seg + idx + 1].time;
    tc = act.time + m.dt; // i(t+dt)

    if (tc < seg_vals[seg].time || tc > seg_vals[seg + len - 1].time)
    {
        seg_vals_res[res_prefix + seg + idx] = PENALTY;
        return;
    }

    tmpj = idx + 1;
    while (tc > tb && ++tmpj < len)
        tb = seg_vals[seg + tmpj].time;

    tmpj = tmpj - 1;

    ta = seg_vals[seg + tmpj].time;
    a = seg_vals[seg + tmpj].ist;
    b = seg_vals[seg + tmpj + 1].ist;

    ipdt = (tc - ta) / (tb - ta) * (b - a) + a;

    right = m.m * ipm + m.n * ipdt;

    switch (metric_type)
    {
    case METRIC_ABS:
    case METRIC_MAX:
        result = fabs(left) - fabs(right);
        break;
    case METRIC_SQ:
        result = left * left - right * right;
        break;
    default:
        result = 0.0f;
        break;
    }

    seg_vals_res[res_prefix + seg + idx] = result;
}

// mapování na seg_vals_res
// max_seg_vals maximální ?as -> x
// seg_count po?et segment? -> y
// get_global_size(0) po?et ?len? -> z

__kernel void solve_avg (const int max_seg_vals, const int seg_count, __global const float* seg_vals_res,
                         __global const int* seg_lenghts, __global float* res, const char metric_type)
{
    const int mem = get_global_id(0);

    int i, j;
    float result = 0.0f;
    float sum;
    const int res_prefix = mem * seg_count * max_seg_vals;

    for (i = 0; i < seg_count; i++)
    {
        sum = 0.0f;
        for (j = 2; j < seg_lenghts[i] - 2; j++)
        {
            sum += seg_vals_res[res_prefix + i * max_seg_vals + j];
        }

        sum /= (seg_lenghts[i] - 4);

        switch (metric_type)
        {
        case METRIC_ABS:
        case METRIC_SQ:
            result = result * i / (i + 1) + sum / (i + 1);
            break;
        case METRIC_MAX:
            result = fmax(sum, result);
            break;
        default:
            return;
        }
    }

    res[mem] = result;
}
