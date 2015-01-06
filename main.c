/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
main.c
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>
#include <float.h>

#include "database.h"
#include "approx.h"
#include "load_ini.h"
#include "evo.h"

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
    int rc = 0;
    int db_size = 0;
    mvalue_ptr *db_values = NULL;
    bounds bconf;
    int metrics[3] = {METRIC_ABS, METRIC_SQ, METRIC_MAX};

    if (argc < 3)
    {
        printf("give more arguments (like db.sqlite3 config.ini)\n");
        return 1;
    }

    // set universal locale (float use .)
    setlocale(LC_NUMERIC,"cs_CZ");

    // don't buff stdout
    setbuf(stdout, NULL);

    // Config
    if (access(argv[2], R_OK) == -1) {
        fprintf(stderr, "bounds config file %s not found\n", argv[2]);
        return -2;
    }

    load_ini(argv[2], &bconf);

    // Database
    if (access(argv[1], R_OK) == -1) {
        fprintf(stderr, "database file %s not found\n", argv[1]);
        return -1;
    }

    rc = init_data(argv[1], &db_values, &db_size);
    if (rc == 0)
        printf("database loaded\n");
    else
        return -1;

    filter(db_values, db_size);

    // TODO: provést paralelní výpočet pro každou metriku a vypsat dobu běhu programu, parametrech použitého algoritmu
    // a vypíše nalezené parametry

    // nakonec vypsat statistiky

/*
    printf("Velikost populace: %d\n", POPULATION_SIZE);
    printf("Počet generací: %d\n", GENERATION_COUNT);
    printf("Mutační konstanta: %f\n", F);
    printf("Práh křížení: %f\n", CR);
*/

    //evolution_serial(db_size, db_values, bconf, METRIC_ABS);
//    for (i = 0; i < 3; i++)
//    {
        evolution_pthread(db_size, db_values, bconf, metrics[0]);
//    }

    // exit and clean up
    free_array(db_values, db_size);

    return 0;
}
