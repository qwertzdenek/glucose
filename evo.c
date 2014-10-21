#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define GENERATION_COUNT 10

#include <math.h>

#include "evo.h"

const float F = 0.75; // mutation constant
const float CR = 0.5; // threshold

void mult(member *a, float f, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) a;

    // member has 12 floats and unknown fitness
    for (i = 0; i < 11; i++)
    {
        *resptr = *aptr * f;
        resptr++;
        aptr++;
    }

    res->fitness = 0.0f;
}

void add(member *a, member *b, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) a;
    float *bptr = (float *) b;

    // member has 12 floats and unknown fitness
    for (i = 0; i < 11; i++)
    {
        *resptr = *bptr + *aptr;
        resptr++;
        aptr++;
        bptr++;
    }

    res->fitness = 0.0f;
}

void diff(member *a, member *b, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) a;
    float *bptr = (float *) b;

    // member has 12 floats and unknown fitness
    for (i = 0; i < 11; i++)
    {
        *resptr = *bptr - *aptr;
        resptr++;
        aptr++;
        bptr++;
    }

    res->fitness = 0.0f;
}

float interp(float val, float a, float b)
{
    return (b - a) * val + a;
}

void cross_m(member *a, member *b, member *res)
{
    int i;
    float rco;
    float *resptr = (float *) res;
    float *aptr = (float *) a;
    float *bptr = (float *) b;

    for (i = 0; i < 11; i++)
    {
        rco = (float) rand() / RAND_MAX;
        *resptr = rco < CR ? *aptr : *bptr;
        resptr++;
        aptr++;
        bptr++;
    }

    res->fitness = 0.0f;
}

#ifdef DEBUG
void print_array(member a[], int s)
{
    int i;

    for (i = 0; i < s; i++)
    {
        printf("[%f %f %f %f %f %f %f %f %f %f %f %f]\n", a[i].p, a[i].cg, a[i].c, a[i].pp, a[i].cgp, a[i].cp, a[i].dt, a[i].h, a[i].k, a[i].m, a[i].n, a[i].fitness);
    }

    printf("\n");
}
#endif // DEBUG


void init_population(member members[], bounds bc)
{
    int i;
    float rco;

    for (i = 0; i < POPULATION_SIZE; i++)
    {
        rco = (float) rand() / RAND_MAX;
        members[i].p = interp(rco, bc.pmax, bc.pmin);
        members[i].cg = interp(rco, bc.cgmax, bc.cgmin);
        members[i].c = interp(rco, bc.cmax, bc.cmin);
        members[i].pp = interp(rco, bc.ppmax, bc.ppmin);
        members[i].cgp = interp(rco, bc.cgpmax, bc.cgpmin);
        members[i].cp = interp(rco, bc.cpmax, bc.cpmin);
        members[i].dt = interp(rco, bc.dtmax, bc.dtmin);
        members[i].h = interp(rco, bc.hmax, bc.hmin);
        members[i].k = interp(rco, bc.kmax, bc.kmin);
        members[i].m = interp(rco, bc.mmax, bc.mmin);
        members[i].n = interp(rco, bc.nmax, bc.nmin);
        members[i].fitness = 0.0f;
    }
}

float fitness(mvalue_ptr *db_values, int db_size, member *m)
{
    int i, j;
    float segment_sum;
    float fit_sum;
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    mvalue act;

    float a, b; // ist vals
    float ta, tb, tc; // interpolated times

    int tmpj; // tmp values for segment searching

    fit_sum = 0;
    for (i = 0; i < db_size; i++)
    {
        segment_sum = 0;
        for (j = 1; j < db_values[i].cvals - 1; j++)
        {
            // phi
            ta = db_values[i].vals[j-1].time;
            tc = db_values[i].vals[j].time - m->h; // i(t - h)

            if (tc < db_values[i].vals[0].time)
                continue;

            tmpj = j - 1;
            while (tc < ta && tmpj > 0)
                ta = db_values[i].vals[--tmpj].time;

            tmpj = tmpj + 1;

            tb = db_values[i].vals[tmpj].time;
            b = db_values[i].vals[tmpj].ist;
            a = db_values[i].vals[tmpj-1].ist;

            itmh = (tc - ta) / (tb - ta) * (b - a) + a; // i(t - h)

            act = db_values[i].vals[j];

            phi = (act.ist - itmh) / m->h;

            // others
            psi = act.blood * (act.blood - act.ist);
            I = m->p * act.blood + m->cg * psi + m->c;
            theta = phi * (m->pp * act.blood + m->cgp * psi + m->cp);

            left = I + theta; // !!!

            // ** RIGHT
            tb = db_values[i].vals[j+1].time;
            tc = db_values[i].vals[j].time + m->dt + m->k*phi;

            tmpj = j + 1;
            while (tc > tb && tmpj < (db_values[i].cvals - 1))
                tb = db_values[i].vals[++tmpj].time;

            tmpj = tmpj - 1;

            ta = db_values[i].vals[tmpj].time;
            a = db_values[i].vals[tmpj].time;
            b = db_values[i].vals[tmpj+1].ist;

            ipm = (tc - ta) / (tb - ta) * (b - a) + a;



            tb = db_values[i].vals[j+1].time;
            tc = act.time + m->dt; // i(t+dt)

            tmpj = j + 1;
            while (tc > tb && tmpj < (db_values[i].cvals - 1))
                tb = db_values[i].vals[++tmpj].time;

            tmpj = tmpj - 1;

            ta = db_values[i].vals[tmpj].time;
            a = db_values[i].vals[tmpj].time;
            b = db_values[i].vals[tmpj+1].ist;

            ipdt = (tc - ta) / (tb - ta) * (b - a) + a;

            right = m->m * ipm + m->n * ipdt;

            segment_sum += right * right - left * left;
        }

        fit_sum += segment_sum / db_values[i].cvals;
    }

    m->fitness = fit_sum;
    printf("member %f: %f -> %f\n", m->p, fit_sum, m->fitness);

    return fit_sum;
}

void evolution(mvalue_ptr *db_values, int db_size, bounds bconf, member members[])
{
    int i, j;
    int a, b, c;
    member op_vec;

    srand(getpid());
    init_population(members, bconf);

    #ifdef DEBUG
    printf("DEBUG: before evolution\n");
    print_array(members, POPULATION_SIZE);
    #endif // DEBUG

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
            b = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
            c = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));

            diff(&members[a], &members[b], &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);              // -> weighted diff vector
            add(&members[c], &op_vec, &op_vec);      // -> noise vector

            // TODO: křížení šumového a cílového cektoru j -> zkušební vektor
            cross_m(&op_vec, &members[j], &op_vec);

            // TODO: oříznout podle mezí

            // fitness zkušebního vektoru a porovnan s cílovým
            float new_fit = fitness(db_values, db_size, &op_vec);
            if (new_fit < members[j].fitness)
                memcpy(&members[j], &op_vec, sizeof(member));

            // vede se statistika nejlepšího jedince v generaci
        }
    }
}
