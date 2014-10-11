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
    int i, j;
    int ncvals;

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

            // TODO: do some aproximation
            val[i].vals[ncvals].ist = 1.0f;

            j++;
            ncvals++;
        }

        printf("segment %d: %d values -> %d\n", i+1, val[i].cvals, ncvals);

        val[i].cvals = ncvals;
    }
}
