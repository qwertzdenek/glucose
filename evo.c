/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include <pthread.h>

#define GENERATION_COUNT 1000

#include <math.h>

#include "evo.h"

const float F = 0.75; // mutation constant
const float CR = 0.3; // threshold

static inline float rfloat();

// database
mvalue_ptr *db_values;
int db_size;
get_metric_func get_metric;
member members[POPULATION_SIZE];
member members_new[POPULATION_SIZE];

#define TRUE 1
#define FALSE 0
#define T_WAIT -1

#define T_COUNT 4
pthread_t workers[T_COUNT];
int tasks[T_COUNT];
int running[T_COUNT];

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

static inline float rfloat()
{
    float f = (float) rand() / RAND_MAX;
    return f;
}

float metric_abs(float left, float right)
{
    return fabs(left) - fabs(right);
}

float metric_square(float left, float right)
{
    return left*left - right*right;
}

void init_population(member members[], bounds bc)
{
    int i;

    for (i = 0; i < POPULATION_SIZE; i++)
    {
        members[i].p = interp(rfloat(), bc.pmax, bc.pmin);
        members[i].cg = interp(rfloat(), bc.cgmax, bc.cgmin);
        members[i].c = interp(rfloat(), bc.cmax, bc.cmin);
        members[i].pp = interp(rfloat(), bc.ppmax, bc.ppmin);
        members[i].cgp = interp(rfloat(), bc.cgpmax, bc.cgpmin);
        members[i].cp = interp(rfloat(), bc.cpmax, bc.cpmin);
        members[i].dt = interp(rfloat(), bc.dtmax, bc.dtmin);
        members[i].h = interp(rfloat(), bc.hmax, bc.hmin);
        members[i].k = interp(rfloat(), bc.kmax, bc.kmin);
        members[i].m = interp(rfloat(), bc.mmax, bc.mmin);
        members[i].n = interp(rfloat(), bc.nmax, bc.nmin);
        members[i].fitness = FLT_MAX;
    }
}

void fitness(member *m)
{
    int i, j;
    float segment_sum;
    float avg_fit;
    float new_fit;
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    mvalue act;

    float a, b; // ist vals
    float ta, tb, tc; // interpolated times

    int sign;

    int tmpj; // tmp values for value searching

    avg_fit = 0;
    for (i = 0; i < db_size; i++)
    {
        segment_sum = 0;

        //printf("i=%d len=%d\n", i, db_values[i].cvals);

        if (db_values[i].cvals < 3)
            continue;

        for (j = 2; j < db_values[i].cvals - 2; j++)
        {
            // phi
            ta = db_values[i].vals[j-1].time;
            tc = db_values[i].vals[j].time - m->h; // i(t - h)

            tmpj = j - 1;
            while (tc <= ta && tmpj > 0)
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
            ta = db_values[i].vals[j].time;
            tc = db_values[i].vals[j].time + m->dt + m->k*phi;

            sign = (int) copysignf(1.0, tc - ta);

            switch (sign)
            {
            case 1:
                tmpj = j + 1;
                tb = db_values[i].vals[tmpj].time;
                while (tc > tb && tmpj < (db_values[i].cvals - 1))
                    tb = db_values[i].vals[++tmpj].time;

                tmpj = tmpj - 1;

                ta = db_values[i].vals[tmpj].time;
                break;

            case -1:
                tmpj = j - 1;
                ta = db_values[i].vals[tmpj].time;
                while (tc < ta && tmpj > 0)
                    ta = db_values[i].vals[--tmpj].time;

                tb = db_values[i].vals[tmpj + 1].time;
                break;
            } // tmpj -> a, tmpj + 1 -> b

            a = db_values[i].vals[tmpj].ist;
            b = db_values[i].vals[tmpj+1].ist;

            ipm = (tc - ta) / (tb - ta) * (b - a) + a;

            tb = db_values[i].vals[j+1].time;
            tc = act.time + m->dt; // i(t+dt)

            tmpj = j + 1;
            while (tc > tb && tmpj < (db_values[i].cvals - 1))
                tb = db_values[i].vals[++tmpj].time;

            tmpj = tmpj - 1;

            ta = db_values[i].vals[tmpj].time;
            a = db_values[i].vals[tmpj].ist;
            b = db_values[i].vals[tmpj+1].ist;

            ipdt = (tc - ta) / (tb - ta) * (b - a) + a;

            right = m->m * ipm + m->n * ipdt;

            segment_sum += get_metric(left, right);
        }

        new_fit = segment_sum / db_values[i].cvals;
        avg_fit = avg_fit * i / (i + 1) + new_fit / (i + 1);
    }

    m->fitness = avg_fit;
}

void evolution_serial(mvalue_ptr *values, int size, bounds bconf, get_metric_func mfun)
{
    int i, j;
    int a, b, c;
    float new_fit;
    member op_vec;
    float min_fit;

    db_values = values;
    db_size = size;
    get_metric = mfun;

    srand(getpid());
    init_population(members, bconf);

    // initialize new generation buffer
    memcpy((member *) members_new, (member *) members, sizeof(member) * POPULATION_SIZE);

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
            b = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
            c = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));

            diff(members + a, members + b, &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);              // -> weighted diff vector
            add(members + c, &op_vec, &op_vec);     // -> noise vector

            cross_m(&op_vec, members + j, &op_vec);

            // fitness zkušebního vektoru a porovnan s cílovým
            fitness(&op_vec);
            new_fit = op_vec.fitness;
            if (fabs(new_fit) < fabs(members[j].fitness))
                memcpy(members_new + j, &op_vec, sizeof(member));
        }

        memcpy((member *) members, (member *) members_new, sizeof(member) * POPULATION_SIZE);
    }

    min_fit = FLT_MAX;
    for (i = 0; i < POPULATION_SIZE; i++)
    {
        if (fabs(members[i].fitness) < fabs(min_fit))
        {
            min_fit = members[i].fitness;
            op_vec = members[i];
        }
    }

    printf("best [%f %f %f %f %f %f %f %f %f %f %f %f]\n",
           op_vec.p, op_vec.cg, op_vec.c, op_vec.pp, op_vec.cgp, op_vec.cp,
           op_vec.dt, op_vec.h, op_vec.k, op_vec.m, op_vec.n, op_vec.fitness);
}

void *work_task(void *par)
{
    int a, b, c;
    float new_fit;
    member op_vec;
    int index = (int) par;
    int act;

    while (running[index])
    {
        // wait for new task
        while (tasks[index] == T_WAIT)
            ;

        act = tasks[index];

        printf("%d computing %d\n", index, act);

        a = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
        b = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));
        c = (int) (((float) rand() / RAND_MAX) * (POPULATION_SIZE - 1));

        diff(members + a, members + b, &op_vec); // -> diff vector
        mult(&op_vec, F, &op_vec);              // -> weighted diff vector
        add(members + c, &op_vec, &op_vec);     // -> noise vector

        cross_m(&op_vec, members + act, &op_vec);

        fitness(&op_vec);
        new_fit = op_vec.fitness;
        if (fabs(new_fit) < fabs(members[act].fitness))
            memcpy(members_new + act, &op_vec, sizeof(member));

        tasks[index] = T_WAIT;
    }

    pthread_exit(NULL);
}

void evolution_pthread(mvalue_ptr *values, int size, bounds bconf, get_metric_func mfun)
{
    int i, j, k;
    float min_fit;
    pthread_attr_t attr;
    member bm;
    int jwait;

    db_values = values;
    db_size = size;
    get_metric = mfun;

    memset(&bm, 0, sizeof(member));

    /* create threads */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    srand(getpid());
    init_population(members, bconf);

    // initialize new generation buffer
    memcpy((member *) members_new, (member *) members, sizeof(member) * POPULATION_SIZE);

    for (k = 0; k < T_COUNT; k++)
    {
        running[k] = TRUE;
        tasks[k] = T_WAIT;
        pthread_create(&workers[k], &attr, work_task, k);
    }

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            jwait = TRUE;

            // check threads
            while (jwait)
            {
                for (k = 0; k < T_COUNT; k++)
                {
                    if (tasks[k] == T_WAIT)
                    {
                        printf("starting %d with %d\n", k, j);
                        tasks[k] = j;
                        jwait = FALSE;
                        break;
                    }
                }
            }
        }

        memcpy((member *) members, (member *) members_new, sizeof(member) * POPULATION_SIZE);
    }

    for (k = 0; k < T_COUNT; k++)
    {
        printf("stopping %d\n", k);
        running[k] = FALSE;
        tasks[k] = k;
        pthread_join(workers[k], NULL);
    }

    pthread_attr_destroy(&attr);

    min_fit = FLT_MAX;
    for (i = 0; i < POPULATION_SIZE; i++)
    {
        if (fabs(members[i].fitness) < fabs(min_fit))
        {
            min_fit = members[i].fitness;
            bm = members[i];
        }
    }

    printf("best [%f %f %f %f %f %f %f %f %f %f %f %f]\n",
           bm.p, bm.cg, bm.c, bm.pp, bm.cgp, bm.cp,
           bm.dt, bm.h, bm.k, bm.m, bm.n, bm.fitness);
}
