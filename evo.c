#include <stdlib.h>
#include <unistd.h>

#define GENERATION_COUNT 10

#include "evo.h"

const float F = 0.75; // mutation constant
const float CR = 0.5; // threshold

void mult(member a, float f, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) &a;

    // member has 12 floats and unknown fitness
    for (i = 0; i < 11; i++)
    {
        *resptr = *aptr * f;
        resptr++;
        aptr++;
    }

    res->fitness = 0.0f;
}

void add(member a, member b, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) &a;
    float *bptr = (float *) &b;

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

void diff(member a, member b, member *res)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) &a;
    float *bptr = (float *) &b;

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

void cross_m(member a, member b, member *res)
{
    int i;
    float rco;
    float *resptr = (float *) res;
    float *aptr = (float *) &a;
    float *bptr = (float *) &b;

    for (i = 0; i < 11; i++)
    {
        rco = (float) rand() / RAND_MAX;
        *resptr = rco < CR ? *aptr : *bptr
        resptr++;
        aptr++;
        bptr++;
    }

    res->fitness = 0.0f;
}

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

float finess(member m)
{
    // TODO
}

void evolution(mvalue_ptr *db_values, int db_size, bounds bconf, member members[])
{
    int i, j;
    int a, b, c;
    member op_vec;

    srand(getpid());
    init_population(members, bconf);

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = (int) ((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1);
            b = (int) ((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1);
            c = (int) ((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1);

            diff(members[a], members[b], &op_vec); // -> diff vector
            mult(op_vec, F, &op_vec);              // -> weighted diff vector
            add(members[c], op_vec, &op_vec);      // -> noise vector

            // TODO: křížení šumového a cílového cektoru j -> zkušební vektor
            cross_m(op_vec, members[j], &op_vec);

            // fitnes zkušebního vektoru a porovnan s cílovým
            float new_fit = fitnes(op_vec);
            if (new_fit < members[j].fitness)
                members[j] = op_vec;

            // vede se statistika nejlepšího jedince v generaci
        }
    }
}
