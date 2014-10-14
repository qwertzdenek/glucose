/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
approx.c
*/

#include <stdio.h>

#include "structures.h"

void filter(mvalue_ptr *val, int cval)
{
    int i, j, k;
    int ncvals;
    float a, b; // ist vals
    float ta, tb, tc; // interpolated times

    for (i = 0; i < cval; i++)
    {
        ncvals = 0;
        j = 0;


        while (j < val[i].cvals)
        {
            while (val[i].vals[j].blood == 0.0f && j < val[i].cvals - 1)
                j++;

            val[i].vals[ncvals].blood = val[i].vals[j].blood;
            val[i].vals[ncvals].time = val[i].vals[j].time;

            /*
            I    B    I
            |----|----|
            ta  tc   tb
            */
            a = val[i].vals[j - 1].ist;
            ta = val[i].vals[j - 1].time;
            tc = val[i].vals[j].time;

            // search for the next ist value
            k = j;
            while (val[i].vals[k].ist == 0.0f && k < val[i].cvals - 1)
                k++;
            b = val[i].vals[k].ist
            tb = val[i].vals[k].time;

            val[i].vals[ncvals].ist = (tc - ta) / (tb - ta) * (b - a) + a;

            j++;
            ncvals++;
        }

        printf("segment %d: %d values -> %d\n", i+1, val[i].cvals, ncvals);

        val[i].cvals = ncvals;
    }
}
