/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
main.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

#include "opencl_target.h"
#include "database.h"
#include "approx.h"
#include "load_ini.h"

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

int main(int argc, char **argv)
{
    int rc = 0;
    int db_size = 0;
    mvalue_ptr *db_values = NULL;
    conf config;

    if (argc < 2)
    {
        printf("give a database name as an argument\n");
        return 1;
    }

    // Config
    if (access(INI_FILE, R_OK) == -1) {
        fprintf(stderr, "config file %s not found\n", INI_FILE);
        return -2;
    }

    load_ini(&config);

    // Database
    if (access(argv[1], R_OK) == -1) {
        fprintf(stderr, "database file %s not found\n", argv[1]);
        return -1;
    }

    // set universal locale (float use .)
    setlocale(LC_NUMERIC,"C");

    rc = init_data(argv[1], &db_values, &db_size);
    if (rc == 0)
        printf("database loaded\n");
    else
        return -1;

    filter(db_values, db_size);

    // TODO: compute best parametres

    // exit and clean up
    free_array(db_values, db_size);

    return 0;
}
