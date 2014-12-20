/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.c
*/

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

//#include "opencl_target.h"
#include "evo.h"
#include "mwc64x_rng.h"
#include "opencl_target.h"

#define GENERATION_COUNT 1000

typedef float (*get_metric_func)(float left, float right);
typedef float (*get_evaluation_func)(float new_val, float old_val, int count);

const float F = 0.75; // mutation constant
const float CR = 0.3; // threshold

// database
mvalue_ptr *db_values;
int db_size;
get_metric_func get_metric;
get_evaluation_func get_eval;
member members[POPULATION_SIZE];
member members_new[POPULATION_SIZE];

#define T_COUNT 4
pthread_t workers[T_COUNT];
pthread_spinlock_t spin;
pthread_barrier_t barrier;
static volatile int worker_counter;

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

void cross_m(member *a, member *b, member *res, uint64_t *s, uint32_t range)
{
    int i;
    float *resptr = (float *) res;
    float *aptr = (float *) a;
    float *bptr = (float *) b;
    uint32_t thresh = CR * range;

    for (i = 0; i < 11; i++)
    {
        *resptr = MWC64X_Next(s, range) < thresh ? *aptr : *bptr;
        resptr++;
        aptr++;
        bptr++;
    }

    res->fitness = FLT_MAX;
}

float metric_abs(float left, float right)
{
    return fabs(left) - fabs(right);
}

float metric_square(float left, float right)
{
    return left*left - right*right;
}

float evaluation_avg(float new_val, float old_val, int i)
{
    return old_val * i / (i + 1) + new_val / (i + 1);
}

float evaluation_max(float new_val, float old_val, int o)
{
    return new_val > old_val ? new_val : old_val;
}

void init_population(member members[], bounds bc, uint64_t *s, uint32_t range)
{
    int i;

    for (i = 0; i < POPULATION_SIZE; i++)
    {
        members[i].p = interp((float) MWC64X_Next(s, range) / range, bc.pmax, bc.pmin);
        members[i].cg = interp((float) MWC64X_Next(s, range) / range, bc.cgmax, bc.cgmin);
        members[i].c = interp((float) MWC64X_Next(s, range) / range, bc.cmax, bc.cmin);
        members[i].pp = interp((float) MWC64X_Next(s, range) / range, bc.ppmax, bc.ppmin);
        members[i].cgp = interp((float) MWC64X_Next(s, range) / range, bc.cgpmax, bc.cgpmin);
        members[i].cp = interp((float) MWC64X_Next(s, range) / range, bc.cpmax, bc.cpmin);
        members[i].dt = interp((float) MWC64X_Next(s, range) / range, bc.dtmax, bc.dtmin);
        members[i].h = interp((float) MWC64X_Next(s, range) / range, bc.hmax, bc.hmin);
        members[i].k = interp((float) MWC64X_Next(s, range) / range, bc.kmax, bc.kmin);
        members[i].m = interp((float) MWC64X_Next(s, range) / range, bc.mmax, bc.mmin);
        members[i].n = interp((float) MWC64X_Next(s, range) / range, bc.nmax, bc.nmin);
        members[i].fitness = FLT_MAX;
    }
}

void fitness(member *m)
{
    int i, j;
    float segment_sum;
    float act_fit;
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    mvalue act;

    float a, b; // ist vals
    float ta, tb, tc; // interpolated times

    int sign;

    int tmpj; // tmp values for value searching

    act_fit = 0;
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

            left = I + theta;

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

        act_fit = get_eval(segment_sum / db_values[i].cvals, act_fit, i);
    }

    m->fitness = act_fit;
}

void evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type)
{
    int i, j;
    int a, b, c;
    float new_fit;
    member op_vec;
    float min_fit;

    db_values = values;
    db_size = num_values;

    switch (metric_type)
    {
    case METRIC_ABS:
        get_metric = metric_abs;
        get_eval = evaluation_avg;
        break;
    case METRIC_SQ:
        get_metric = metric_square;
        get_eval = evaluation_avg;
        break;
    case METRIC_MAX:
        get_metric = metric_abs;
        get_eval = evaluation_max;
        break;
    default:
        return;
    }

    uint64_t state;
    uint32_t range = 12*POPULATION_SIZE;

    MWC64X_Seed(&state, range, time(NULL));
    init_population(members, bconf, &state, range);

    // initialize new generation buffer
    memcpy((member *) members_new, (member *) members, sizeof(member) * POPULATION_SIZE);

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        range = 4*POPULATION_SIZE;
        MWC64X_Seed(&state, range, time(NULL));

        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = MWC64X_Next(&state, range) / 4;
            b = MWC64X_Next(&state, range) / 4;
            c = MWC64X_Next(&state, range) / 4;

            diff(members + a, members + b, &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);              // -> weighted diff vector
            add(members + c, &op_vec, &op_vec);     // -> noise vector

            cross_m(&op_vec, members + j, &op_vec, &state, range);

            // fitness zkušebního vektoru a porovnám s cílovým
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
    int i, j;
    int a, b, c;
    float new_fit;
    member op_vec;
    long index = (long) par;

    uint32_t range = 4*POPULATION_SIZE;
    uint64_t state;

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        MWC64X_Seed(&state, range, 6803 * (i + 1));

        for (j = index; j < POPULATION_SIZE; j = j + T_COUNT)
        {
            a = MWC64X_Next(&state, range) / 4;
            b = MWC64X_Next(&state, range) / 4;
            c = MWC64X_Next(&state, range) / 4;

            diff(members + a, members + b, &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);              // -> weighted diff vector
            add(members + c, &op_vec, &op_vec);     // -> noise vector

            cross_m(&op_vec, members + j, &op_vec, &state, range);

            fitness(&op_vec);
            new_fit = op_vec.fitness;
            if (fabs(new_fit) < fabs(members[j].fitness))
                memcpy(members_new + j, &op_vec, sizeof(member));
        }

        pthread_spin_lock(&spin);
        worker_counter++;

        if (worker_counter >= T_COUNT)
        {
            memcpy((member *) members, (member *) members_new, sizeof(member) * POPULATION_SIZE);
            worker_counter = 0;
        }
        pthread_spin_unlock(&spin);

        pthread_barrier_wait(&barrier);
    }

    pthread_exit(NULL);
}

void evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type)
{
    int i;
    long k;
    float min_fit;
    pthread_attr_t attr;
    member bm;

    db_values = values;
    db_size = num_values;

    switch (metric_type)
    {
    case METRIC_ABS:
        get_metric = metric_abs;
        get_eval = evaluation_avg;
        break;
    case METRIC_SQ:
        get_metric = metric_square;
        get_eval = evaluation_avg;
        break;
    case METRIC_MAX:
        get_metric = metric_abs;
        get_eval = evaluation_max;
        break;
    default:
        return;
    }

    uint64_t state;
    uint32_t range = 12*POPULATION_SIZE;

    MWC64X_Seed(&state, range, time(NULL));
    init_population(members, bconf, &state, range);

    // initialize new generation buffer
    memcpy((member *) members_new, (member *) members, sizeof(member) * POPULATION_SIZE);

    // create threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    worker_counter = 0;
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    pthread_barrier_init(&barrier, NULL, T_COUNT);

    for (k = 0; k < T_COUNT; k++)
    {
        pthread_create(&workers[k], &attr, work_task, (void *) k);
    }

    for (k = 0; k < T_COUNT; k++)
    {
        pthread_join(workers[k], NULL);
    }

    pthread_attr_destroy(&attr);
    pthread_spin_destroy(&spin);
    pthread_barrier_destroy(&barrier);

    memset(&bm, 0, sizeof(member));
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

void evolution_opencl(int num_values, mvalue_ptr *values, bounds bconf, int metric_type)
{
    int i, j;
    int a, b, c;
    member op_vec;
    float min_fit;

    uint64_t state;
    uint32_t range = 12*POPULATION_SIZE;

    init_opencl(num_values, values, metric_type);

    MWC64X_Seed(&state, range, time(NULL));
    init_population(members, bconf, &state, range);

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        range = 4*POPULATION_SIZE;
        MWC64X_Seed(&state, range, time(NULL));

        // nageneruje nové kandidáty populace
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = MWC64X_Next(&state, range) / 4;
            b = MWC64X_Next(&state, range) / 4;
            c = MWC64X_Next(&state, range) / 4;

            diff(members + a, members + b, &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);              // -> weighted diff vector
            add(members + c, &op_vec, &op_vec);     // -> noise vector

            cross_m(&op_vec, members + j, &op_vec, &state, range);

            memcpy(members_new + j, &op_vec, sizeof(member));
        }
        // spočítej přes opencl fitness pro každého člena v members_new
        cl_compute_fitness(POPULATION_SIZE, members_new);

        // nahraď v members lepší členy z members_new
        for (j = 0; j < POPULATION_SIZE; j++)
        {
            if (members[j].fitness > members_new[j].fitness)
                members[j] = members_new[j];
        }
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
