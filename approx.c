/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
approx.c
*/

#include <stdio.h>
#include <math.h>

#include "structures.h"

void filter(mvalue_ptr *val, int db_size)
{
    int i, j;
    int ncvals;
    int last_ist;
    int next_ist;

    float a, b; // ist vals
    float ta, tb, tc; // interpolated times
    float ist;

    for (i = 0; i < db_size; i++)
    {
        ncvals = 0;
        j = 1;
        last_ist = -1;
        next_ist = -1;

        // insert left bound value because of aproximation
        val[i].vals[0].time = val[i].vals[0].time - 365.0;
        val[i].vals[0].blood = 4.0f;
        val[i].vals[0].ist = 4.0f;
        ncvals++;

        // skip first blood values without ist
        while (j < val[i].cvals && val[i].vals[j].blood > 0.0f)
            j++;

        while (j < val[i].cvals)
        {
            // search for the blood value
            while (j < val[i].cvals && val[i].vals[j].blood == 0.0f)
            {
                last_ist = j;
                j++;
            } // -> j - blood index, last_ist - ist value from the left side

            // TODO: check if value i is already there

            // search for the next ist value
            next_ist = j + 1;
            while (next_ist < val[i].cvals && val[i].vals[next_ist].ist == 0.0f)
                next_ist++;

            if (next_ist >= val[i].cvals)
                break;

            val[i].vals[ncvals].blood = val[i].vals[j].blood;
            val[i].vals[ncvals].time = val[i].vals[j].time;

            /*
            I    B    I
            |----|----|
            ta  tc   tb
            */
            a = val[i].vals[last_ist].ist;
            ta = val[i].vals[last_ist].time;
            b = val[i].vals[next_ist].ist;
            tb = val[i].vals[next_ist].time;

            tc = val[i].vals[j].time;

            ist = (tc - ta) / (tb - ta) * (b - a) + a;

            if (!isnan(ist))
                val[i].vals[ncvals++].ist = ist;

            j++;
        }

        // insert left bound value because of aproximation
        val[i].vals[ncvals].time = val[i].vals[j-1].time + 365;
        val[i].vals[ncvals].blood = 4.0f;
        val[i].vals[ncvals].ist = 4.0f;
        ncvals++;

        val[i].cvals = ncvals;
    }
}
