/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
database.c
*/

#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

#include "database.h"

mvalue_ptr *db_private;

/**
 * time string to parse
 * tm result structure
 */
double iso2double(const char *time) {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    strptime(time, "%Y-%m-%dT%H:%M:%S", &tm);
    time_t tval = mktime(&tm);

    const double secsPerDay = 24.0*60.0*60.0;
    const double invSecsPerDay = 1.0 / secsPerDay;

    return ((double) tval)*invSecsPerDay;
}

/* callbacks for SQLite3 */
int cb_value(void *id_ptr, int argc, char **argv, char **azColName)
{
    int id = *((int *) id_ptr);
    int vc = db_private[id].cvals;
    char *endptr;

    if (argc < 3)
    {
        printf("argc was %d", argc);
        return -1;
    }

    db_private[id].vals[vc].time = iso2double(argv[0]);
    db_private[id].vals[vc].blood = argv[1] == NULL ? 0.0f : strtof(argv[1], &endptr);
    db_private[id].vals[vc].ist = argv[2] == NULL ? 0.0f : strtof(argv[2], &endptr);

    // printf("%d: %.5f blood=%.2f ist=%.2f\n", id, db_private[id].vals[vc].time, db_private[id].vals[vc].blood, db_private[id].vals[vc].ist);

    db_private[id].cvals++;

    return 0;
}

int cb_size(void *res, int argc, char **argv, char **azColName)
{
    char *err_ptr = NULL;
    *((int *) res) = (int) strtol(argv[0], &err_ptr, 10);

    if (argv[0] == err_ptr) return -1;

    return 0;
}

int init_data(char *file, mvalue_ptr **db_values, int *cval)
{
    int i;
    sqlite3 *db;
    char *zErrMsg = 0;
    char query[128];
    int rc;
    int query_size = 0;

    rc = sqlite3_open(file, &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    // count of measured segments
    rc = sqlite3_exec(db, "select count(distinct segmentid) from measuredvalue;", cb_size, cval, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }

    // alloc array
    *db_values = (mvalue_ptr *) malloc(*cval * sizeof(mvalue_ptr));
    db_private = *db_values;

    // load data
    for (i = 0; i < *cval; i++)
    {
        sprintf((char *) &query, "SELECT count(measuredat) FROM measuredvalue WHERE segmentid==%d GROUP BY segmentid;", i + 1);
        sqlite3_exec(db, (char *) query, cb_size, &query_size, NULL);

        db_private[i].vals = (mvalue *) malloc(query_size * sizeof(mvalue));
        db_private[i].cvals = 0;

        sprintf((char *) &query, "SELECT measuredat, blood, ist FROM measuredvalue WHERE segmentid==%d;", i + 1);
        rc = sqlite3_exec(db, (char *) query, cb_value, &i, &zErrMsg);
        if(rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }

    sqlite3_close(db);
    return 0;
}
