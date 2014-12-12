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

#include "opencl_target.h"
#include "database.h"
#include "approx.h"
#include "load_ini.h"
#include "evo.h"

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

void print_array(member a[], int s)
{
    int i;

    for (i = 0; i < s; i++)
    {
        printf("[%f %f %f %f %f %f %f %f %f %f %f %f]\n", a[i].p, a[i].cg, a[i].c, a[i].pp, a[i].cgp, a[i].cp, a[i].dt, a[i].h, a[i].k, a[i].m, a[i].n, a[i].fitness);
    }

    printf("\n");
}

int main(int argc, char **argv)
{
    int rc = 0;
    int db_size = 0;
    mvalue_ptr *db_values = NULL;
    bounds bconf;

    if (argc < 2)
    {
        printf("give a database name as an argument\n");
        return 1;
    }

    // set universal locale (float use .)
    setlocale(LC_NUMERIC,"C");

    // Config
    if (access(INI_FILE, R_OK) == -1) {
        fprintf(stderr, "bounds config file %s not found\n", INI_FILE);
        return -2;
    }

    load_ini(&bconf);

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

//    init_opencl();

    //evolution_serial(db_values, db_size, bconf, metric_abs);
    //evolution_serial(db_values, db_size, bconf, metric_square);

    evolution_pthread(db_values, db_size, bconf, metric_abs);
    //evolution_pthread(db_values, db_size, bconf, metric_square);

    //print_array(members, POPULATION_SIZE);

    // exit and clean up
    free_array(db_values, db_size);

    return 0;
}
