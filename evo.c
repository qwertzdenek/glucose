/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
evo.c
*/

#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

#include "evo.h"
#include "mwc64x_rng.h"
#include "opencl_target.h"

// Windows hack
#ifdef __MINGW32__
#ifndef _SC_NPROCESSORS_ONLN
SYSTEM_INFO info;
GetSystemInfo(&info);
#define sysconf(a) info.dwNumberOfProcessors
#define _SC_NPROCESSORS_ONLN
#endif
#endif

// function pointers for different metric computation
typedef float (*get_metric_func)(float left, float right);
typedef float (*get_evaluation_func)(float new_val, float old_val, int count);

// database
mvalue_ptr *db_values;
int db_size;

get_metric_func get_metric;
get_evaluation_func get_eval;

// swap buffers for the evolution
member members[POPULATION_SIZE];
member members_new[POPULATION_SIZE];

// threading
int cpu_count;
pthread_t *workers;
pthread_spinlock_t spin;
pthread_barrier_t barrier;
static volatile int worker_counter;
static uint64_t seed;

/**
 * vector multiplication by float constant
 * param a first vector
 * param f constant
 * param res result
 */
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

/**
 * vector addition
 * param a first vector
 * param b second vector
 * param res result
 */
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

/**
 * vector division
 * param a first vector
 * param b second vector
 * param res result
 */
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

/**
 * linear interpolation between a and b by val
 * param val interpolation coeficient. It should be between 0 and 1.
 * param a first value
 * param b second value
 */
float interp(float val, float a, float b)
{
    return (b - a) * val + a;
}

/**
 * It does cross of vector a and b.
 * param a first member
 * param b second member
 * param res result member
 * param s RNG seed. Initialize first.
 * param range number range of the RNG
 */
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

/**
 * Computes difference of absolute values from the left and right side.
 * param left left side of the equation
 * param right right side of the equation
 */
float metric_abs(float left, float right)
{
    return fabsf(left) - fabsf(right);
}

/**
 * Computes square value from the left and right side.
 * param left left side of the equation
 * param right right side of the equation
 */
float metric_square(float left, float right)
{
    return left*left - right*right;
}

/**
 * Computes continuesly added average.
 * param new_val value added to the average
 * param old_val old value of the average
 * param i actual size of the sequence
 */
float evaluation_avg(float new_val, float old_val, int i)
{
    return old_val * i / (i + 1) + new_val / (i + 1);
}

/**
 * Returns the bigger of old and new value.
 * param new_val new value
 * param old_val old value
 * param o not used parameter (API need it)
 */
float evaluation_max(float new_val, float old_val, int o)
{
    return new_val > old_val ? new_val : old_val;
}

/**
 * Tests if time is between low and high edge.
 * param time tested time value
 * param low low edge of window
 * param high high edge of window
 */
inline int test_edge(float time, float low, float high)
{
    return time < low || time > high;
}

void print_progress(float percent)
{
    printf("\r%.2f %%", percent);
}

/**
 * prints values of all members in the array a. Member is an array of equation parameters.
 * param a an array
 * param s size of the array
 */
void print_array(member a[], int s)
{
    int i;

    for (i = 0; i < s; i++)
    {
        printf("[%f %f %f %f %f %f %f %f %f %f %f %f]\n", a[i].p, a[i].cg, a[i].c, a[i].pp, a[i].cgp, a[i].cp, a[i].dt, a[i].h, a[i].k, a[i].m, a[i].n, a[i].fitness);
    }

    printf("\n");
}

/**
 * Fills members array with initial setup
 * param members an array of strucs to fill
 * param bc basic chromosome pattern
 * param s RNG seed value
 * param range number range of the RNG
 */
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

/**
 * Fitness function
 */
void fitness(member *m)
{
    int i, j;
    float segment_sum;
    float act_fit;
    float phi, psi, I, theta, left, right;
    float itmh, ipm, ipdt;
    mvalue act;
    mvalue_ptr el;

    float a, b; // ist vals
    float ta, tb, tc; // interpolated times

    int sign;

    int tmpj; // tmp values for value searching

    act_fit = 0;
    for (i = 0; i < db_size; i++) // iterate segments
    {
        segment_sum = 0;
        el = db_values[i];

        if (el.cvals < 5)
            continue;

        for (j = 2; j < el.cvals - 2; j++) // iterate time
        {
            act = el.vals[j];

            // phi
            ta = el.vals[j-1].time;
            tc = act.time - m->h; // i(t - h)

            if (test_edge(tc, el.vals[0].time, el.vals[el.cvals - 1].time))
            {
                segment_sum += PENALTY;
                continue;
            }

            tmpj = j - 1;
            while (tc <= ta && --tmpj >= 0)
                ta = el.vals[tmpj].time;

            tmpj = tmpj + 1;

            tb = el.vals[tmpj].time;
            b = el.vals[tmpj].ist;
            a = el.vals[tmpj-1].ist;

            itmh = (tc - ta) / (tb - ta) * (b - a) + a; // i(t - h)

            phi = (act.ist - itmh) / m->h;

            // others
            psi = act.blood * (act.blood - act.ist);
            I = m->p * act.blood + m->cg * psi + m->c;
            theta = phi * (m->pp * act.blood + m->cgp * psi + m->cp);

            left = I + theta;

            // ** RIGHT
            ta = el.vals[j].time;
            tc = el.vals[j].time + m->dt + m->k*phi;

            if (test_edge(tc, el.vals[0].time, el.vals[el.cvals - 1].time))
            {
                segment_sum += PENALTY;
                continue;
            }

            sign = (int) copysignf(1.0, tc - ta);

            switch (sign)
            {
            case 1:
                tmpj = j + 1;
                tb = el.vals[tmpj].time;
                while (tc > tb && ++tmpj < el.cvals)
                    tb = el.vals[tmpj].time;

                tmpj = tmpj - 1;
                ta = el.vals[tmpj].time;
                break;

            case -1:
                tmpj = j - 1;
                ta = el.vals[tmpj].time;
                while (tc < ta && --tmpj >= 0)
                    ta = el.vals[tmpj].time;

                tb = el.vals[tmpj + 1].time;
                break;
            } // tmpj -> a, tmpj + 1 -> b

            a = el.vals[tmpj].ist;
            b = el.vals[tmpj+1].ist;

            ipm = (tc - ta) / (tb - ta) * (b - a) + a;

            tb = el.vals[j+1].time;
            tc = act.time + m->dt; // i(t+dt)

            if (test_edge(tc, el.vals[0].time, el.vals[el.cvals - 1].time))
            {
                segment_sum += PENALTY;
                continue;
            }

            tmpj = j + 1;
            while (tc > tb && ++tmpj < el.cvals)
                tb = el.vals[tmpj].time;

            tmpj = tmpj - 1;

            ta = el.vals[tmpj].time;
            a = el.vals[tmpj].ist;
            b = el.vals[tmpj+1].ist;

            ipdt = (tc - ta) / (tb - ta) * (b - a) + a;

            right = m->m * ipm + m->n * ipdt;

            segment_sum += get_metric(left, right);
        }

        act_fit = get_eval(segment_sum / (el.cvals - 4), act_fit, i);
    }

    m->fitness = act_fit;
}

/**
 * Serial version of differencial evolution
 * param num_values size of the database
 * param values
 */
int evolution_serial(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result)
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
        return EVO_ERROR;
    }

    uint64_t state;
    uint32_t range = 12*POPULATION_SIZE;

    printf("** running serial version **\n");

    MWC64X_Seed(&state, range, time(NULL));
    init_population(members, bconf, &state, range);

    // initialize new generation buffer
    memcpy((member *) members_new, (member *) members, sizeof(member) * POPULATION_SIZE);

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        range = POPULATION_SIZE;
        MWC64X_Seed(&state, range, time(NULL));

        for (j = 0; j < POPULATION_SIZE; j++)
        {
            a = MWC64X_Next(&state, range);
            b = MWC64X_Next(&state, range);
            c = MWC64X_Next(&state, range);

            diff(members + a, members + b, &op_vec); // -> diff vector
            mult(&op_vec, F, &op_vec);               // -> weighted diff vector
            add(members + c, &op_vec, &op_vec);      // -> noise vector

            cross_m(&op_vec, members + j, &op_vec, &state, range);

            // fitness zkušebního vektoru a porovnám s cílovým
            fitness(&op_vec);
            new_fit = op_vec.fitness;
            if (fabs(new_fit) < fabs(members[j].fitness))
                memcpy(members_new + j, &op_vec, sizeof(member));
        }

        print_progress(100*((float) i / GENERATION_COUNT));

        memcpy((member *) members, (member *) members_new, sizeof(member) * POPULATION_SIZE);
    }

    printf("\n\n");

    min_fit = FLT_MAX;
    for (i = 0; i < POPULATION_SIZE; i++)
    {
        if (fabs(members[i].fitness) < fabs(min_fit))
        {
            min_fit = members[i].fitness;
            op_vec = members[i];
        }
    }

    *result = op_vec;
    return EVO_OK;
}

void *work_task(void *par)
{
    int i, j;
    int a, b, c;
    float new_fit;
    member op_vec;
    long index = (long) par;

    uint32_t range = POPULATION_SIZE;
    uint64_t state;

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        MWC64X_Seed(&state, range, seed + 100 * i);

        for (j = index; j < POPULATION_SIZE; j = j + cpu_count)
        {
            a = MWC64X_Next(&state, range);
            b = MWC64X_Next(&state, range);
            c = MWC64X_Next(&state, range);

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

        if (worker_counter >= cpu_count)
        {
            memcpy((member *) members, (member *) members_new, sizeof(member) * POPULATION_SIZE);
            worker_counter = 0;
            print_progress(100*((float) i / GENERATION_COUNT));
        }
        pthread_spin_unlock(&spin);

        pthread_barrier_wait(&barrier);
    }

    pthread_exit(NULL);
}

int evolution_pthread(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result)
{
    int i;
    long k;
    float min_fit;
    pthread_attr_t attr;
    member op_vec;

    db_values = values;
    db_size = num_values;

    #ifdef _SC_NPROCESSORS_ONLN
    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count < 1)
    {
        fprintf(stderr, "Could not determine number of CPUs online:\n");
        return 0.0f;
    }
    #else
    fprintf(stderr, "Could not determine number of CPUs");
    return 0.0f;
    #endif

    workers = (pthread_t *) malloc(cpu_count * sizeof(pthread_t));

    printf("** running pthread version with %d threads **\n", cpu_count);

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
        return EVO_ERROR;
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

    seed = time(NULL);
    worker_counter = 0;
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    pthread_barrier_init(&barrier, NULL, cpu_count);

    for (k = 0; k < cpu_count; k++)
    {
        pthread_create(workers + k, &attr, work_task, (void *) k);
    }

    for (k = 0; k < cpu_count; k++)
    {
        pthread_join(workers[k], NULL);
    }

    pthread_attr_destroy(&attr);
    pthread_spin_destroy(&spin);
    pthread_barrier_destroy(&barrier);
    free(workers);

    printf("\n\n");

    memset(&op_vec, 0, sizeof(member));
    min_fit = FLT_MAX;
    for (i = 0; i < POPULATION_SIZE; i++)
    {
        if (fabs(members[i].fitness) < fabs(min_fit))
        {
            min_fit = members[i].fitness;
            op_vec = members[i];
        }
    }

    *result = op_vec;
    return EVO_OK;
}

int evolution_opencl(int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *result)
{
    int i;
    member op_vec;
    float min_fit;

    memset((void *) &op_vec, 0, sizeof(member));

    uint64_t state;
    uint32_t range = 12*POPULATION_SIZE;

    printf("** running OpenCL version **\n");

    MWC64X_Seed(&state, range, time(NULL));
    init_population(members, bconf, &state, range);

    if (cl_init(num_values, values, POPULATION_SIZE, members, metric_type) == OPENCL_ERROR)
        return EVO_ERROR;

    for (i = 0; i < GENERATION_COUNT; i++)
    {
        // spočítej přes opencl fitness pro každého člena v members
        cl_compute_fitness(time(NULL));

        print_progress(100*((float) i / GENERATION_COUNT));
    }

    printf("\n\n");

    cl_read_result(members);
    cl_cleanup();

    min_fit = FLT_MAX;
    for (i = 0; i < POPULATION_SIZE; i++)
    {
        if (fabs(members[i].fitness) < fabs(min_fit))
        {
            min_fit = members[i].fitness;
            op_vec = members[i];
        }
    }

    *result = op_vec;
    return EVO_OK;
}
