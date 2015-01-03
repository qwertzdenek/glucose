/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
approx.c
*/

#include <math.h>
#include <string.h>

#include "structures.h"

/**
 * this internal method approximates only missing blood values
 * param vals pointer of the first array element
 * param len length of the array
 */
void approx_blood(mvalue *vals, int len)
{
    int i;
    int last, next;
    float a, b; // blood vals
    float ta, tb, tc; // time

    last = 0;
    i = 1;
    while (i < len - 1)
    {
        // search for the missing blood value
        while (i < len && vals[i].blood > 0.0f)
        {
            last = i;
            i++;
        } // -> i - blood index, last - last index from the left side

        // search for the next blood value
        next = i + 1;
        while (next < len && vals[next].blood == 0.0f)
            next++;

        /*
        B    N    B
        |----|----|
        ta  tc   tb
        */
        a = vals[last].blood;
        ta = vals[last].time;
        b = vals[next].blood;
        tb = vals[next].time;

        tc = vals[i].time;

        vals[i].blood = (tc - ta) / (tb - ta) * (b - a) + a;

        i++;
    }
}

/**
 * This internal method approximates only missing ist values.
 * param vals pointer of the first array element
 * param len length of the array
 */
void approx_ist(mvalue *vals, int len)
{
    int i;
    int last, next;
    float a, b; // ist vals
    float ta, tb, tc; // time

    last = 0;
    i = 1;
    while (i < len - 1)
    {
        // search for the missing ist value
        while (i < len && vals[i].ist > 0.0f)
        {
            last = i;
            i++;
        } // -> i - ist index, last - last index from the left side

        // search for the next ist value
        next = i + 1;
        while (next < len && vals[next].ist == 0.0f)
            next++;

        /*
        I    N    I
        |----|----|
        ta  tc   tb
        */
        a = vals[last].ist;
        ta = vals[last].time;
        b = vals[next].ist;
        tb = vals[next].time;

        tc = vals[i].time;

        vals[i].ist = (tc - ta) / (tb - ta) * (b - a) + a;

        i++;
    }
}

/**
 * This function finds missing values and finds their linear interpolation.
 * param val all segments in the database
 * param db_size segment database size
 */
void filter(mvalue_ptr *val, int db_size)
{
    int i, j;
    int ncvals;
    int index_one;
    int index_two;
    int index;

    for (i = 0; i < db_size; i++)
    {
        // insert left bound value because of aproximation
        val[i].vals[0].time = val[i].vals[0].time - 42.0f; // keep edges safe
        val[i].vals[0].blood = 4.0f;
        val[i].vals[0].ist = 4.0f;

        j = 1;
        while (j < val[i].cvals && val[i].vals[j].ist == 0.0f)
            j++;
        index_one = j;

        j = 1;
        while (j < val[i].cvals && val[i].vals[j].blood == 0.0f)
            j++;
        index_two = j;

        // frame begin
        index = fmaxl(index_one, index_two);

        j = val[i].cvals - 1;
        while (j > 0 && val[i].vals[j].ist == 0.0f)
            j--;
        index_one = j;

        j = val[i].cvals - 1;
        while (j > 0 && val[i].vals[j].blood == 0.0f)
            j--;
        index_two = j;

        // frame end
        index_two = fminl(index_one, index_two);

        ncvals = index_two - index + 1;

        // memmove of valid frame to the beginng
        memmove(val[i].vals + 1, val[i].vals + index, ncvals * sizeof(mvalue));

        ncvals += 2;

        // insert left bound value because of aproximation
        val[i].vals[ncvals - 1].time = val[i].vals[j].time + 42.0f; // keep edges safe
        val[i].vals[ncvals - 1].blood = 4.0f;
        val[i].vals[ncvals - 1].ist = 4.0f;

        val[i].cvals = ncvals;

        approx_blood(val[i].vals, ncvals);
        approx_ist(val[i].vals, ncvals);
    }
}
