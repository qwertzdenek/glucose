/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
OpenCL evolution
*/

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

__kernel void solve_equation (const int num_seg_vals, __global const mvalue* seg_vals,
    __global const int* seg_lenghts, const member m, __global float* seg_vals_res, const char metric_type)
{
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    float a, b; // ist vals
    float ta, tb, tc; // interpolated times
		float result;
    int s, tmpj;
    mvalue act;

    const int idx = get_global_id(0);
    const int idy = get_global_id(1);
    const int seg = idy * get_global_size(0);
		const int len = seg_lenghts[idy];

	  if (idy >= num_seg_vals)
				return;

    if (idx < 2)
        return;

		if (idx >= len - 2)
				return;

    act = seg_vals[seg + idx];

    ta = seg_vals[seg + idx - 1].time;
    tc = act.time - m.h; // i(t - h)

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

		seg_vals_res[seg + idx] = result;
}

__kernel void solve_avg (const int max_seg_vals, __global const float* seg_vals_res,
    __global const int* seg_lenghts, __global float* res)
{
		int i;
		float result = 0;
    const int seg = get_global_id(0);
	  const int offset = seg * max_seg_vals;

	  for (i = 2; i < seg_lenghts[seg] - 2; i++)
		{
			  result += seg_vals_res[offset + i];
		}
		
		res[seg] = result / (seg_lenghts[seg] - 4);
}
