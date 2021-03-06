/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
main.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include "database.h"
#include "approx.h"
#include "load_ini.h"
#include "evo.h"

int num_generations;

int metrics[3] = {METRIC_ABS, METRIC_SQ, METRIC_MAX};
char *metrics_name[3] = {"absolute", "square", "maximum"};
char *methods[3] = {"serial", "pthread", "OpenCL"};

char *help = "Glucose transportation simulation && Zdeněk Janeček 2015\n"\
"use: glucose DATABASE CONFIG METHOD GENERATIONS METRIC\n"\
"  METHOD:\n"\
"    -s serial version\n"\
"    -p pthread version\n"\
"    -c opencl version\n"\
"  METRIC:\n"\
"    -a absolute value\n"\
"    -s squared value\n"\
"    -m maximum difference\n"\
"  GENERATIONS means count of computed generations\n"\
"\n"\
"example: glucose db.sqlite3 config.ini -p 500 -s\n";

/**
 * frees segments in array
 * param array pointer to the first array element
 * param size count of segments in the
 */
void free_array(mvalue_ptr *array, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        free(array[i].vals);
    }

    if (size > 0)
        free(array);
}

/**
 * swap two elements of the array
 * param array pointer to the first array element
 * param size count of segments in the array
 */
void swap(mvalue_ptr *array, int i, int j)
{
    mvalue_ptr tmp;
    tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}

int main(int argc, char **argv)
{
    int metric;
    int method;
    int csv = 0;
    int db_size = 0;
    FILE *f;
    mvalue_ptr *db_values = NULL;
    bounds bconf;
    member r;
    int (*evolution) (int num_values, mvalue_ptr *values, bounds bconf, int metric_type, member *r, double *time_used);
    double took;
    int err;

    if (argc < 6 || strcmp("-h", argv[1]) == 0)
    {
        printf(help);
        return 0;
    }

    // set universal locale (float use .)
    setlocale(LC_NUMERIC,"cs_CZ");

    // don't buff stdout
    setbuf(stdout, NULL);

    // Config
    if ((f = fopen(argv[2], "r")) == NULL)
    {
        fprintf(stderr, "bounds config file %s not found\n", argv[2]);
        return -2;
    }
    fclose(f);

    // Database
    if ((f = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "database file %s not found\n", argv[1]);
        return -1;
    }
    fclose(f);

    if (strcmp("-s", argv[3]) == 0)
    {
        evolution = evolution_serial;
        method = 0;
    }
    else if (strcmp("-p", argv[3]) == 0)
    {
        evolution = evolution_pthread;
        method = 1;
    }
    else if (strcmp("-c", argv[3]) == 0)
    {
        evolution = evolution_opencl;
        method = 2;
    }
    else
    {
        fprintf(stderr, "unknown method %s\n", argv[3]);
        return -3;
    }

    num_generations = strtol(argv[4], NULL, 10);

    if (strcmp("-a", argv[5]) == 0)
        metric = 0;
    else if (strcmp("-s", argv[5]) == 0)
        metric = 1;
    else if (strcmp("-m", argv[5]) == 0)
        metric = 2;
    else
    {
        fprintf(stderr, "unknown metric %s\n", argv[5]);
        return -4;
    }

    load_ini(argv[2], &bconf);

    if (init_data(argv[1], &db_values, &db_size))
        return -1;

    if (argc == 7 && strcmp("-csv", argv[6]) == 0)
        csv = 1;

    filter(db_values, db_size);

    if (csv)
    {
        err = evolution(db_size, db_values, bconf, metrics[metric], &r, &took);
        if (err == EVO_OK)
            printf("%s;%s;%.4f;%.6f;%d;%d\n", methods[method], metrics_name[metric], took, r.fitness, num_generations, POPULATION_SIZE);
    }
    else
    {
        printf("** running %s version **\n", methods[method]);
        printf("Population size: %d\n", POPULATION_SIZE);
        printf("Generation count: %d\n", num_generations);
        printf("Mutation constant F: %.2f\n", F);
        printf("Cross threshold CR: %.2f\n", CR);

        err = evolution(db_size, db_values, bconf, metrics[metric], &r, &took);

        if (err == EVO_OK)
        {
            printf("Finished %s %s metric with fitness %.4f.\n", methods[method], metrics_name[metric], r.fitness);
            #ifndef __MINGW32__
            printf("Operation took %.2f seconds.\n", took);
            #endif
            printf("Found parametres:\np=%f\ncg=%f\nc=%f\npp=%f\ncgp=%f\ncp=%f\ndt=%f\nh=%f\nk=%f\nm=%f\nn=%f\n",
                  r.p,  r.cg,  r.c,  r.pp,  r.cgp,  r.cp,  r.dt,  r.h,  r.k,  r.m,  r.n);
        }
    }

    // exit and clean up
    free_array(db_values, db_size);

    return 0;
}
