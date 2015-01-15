/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
database.c
*/

#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sqlite3.h"
#include "database.h"

mvalue_ptr *db_private;

/**
 * converts ISO time formatted string to the double value.
 * Day has value of 1. It starts in the year 1995.
 *
 * param time string to parse
 * return time in double
 */
double iso2double(const char *time) {
    int year, month, date, hour, minute, seconds;
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    #ifdef __MINGW32__
    sscanf(time, "%d-%d-%dT%d:%d:%d", &year, &month, &date, &hour, &minute, &seconds);
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = date;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = seconds;
    #else
    strptime(time, "%Y-%m-%dT%H:%M:%S", &tm);
    #endif
    time_t tval = mktime(&tm);

    const double secsPerDay = 24.0*60.0*60.0;
    const time_t secsPer25years = 788400000;
    const double invSecsPerDay = 1.0 / secsPerDay;
    tval -= secsPer25years;

    return ((double) tval)*invSecsPerDay;
}

/**
 * callback for SQLite3. It parse one row and saves it to the db_private array.
 */
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

    db_private[id].cvals++;

    return 0;
}

/**
 * callback for SQLite3. It parse count of rows and saves result to the res.
 */
int cb_size(void *res, int argc, char **argv, char **azColName)
{
    char *err_ptr = NULL;
    *((int *) res) = (int) strtol(argv[0], &err_ptr, 10);

    if (argv[0] == err_ptr) return -1;

    return 0;
}

/**
 * This method does database queries and fills arguments.
 * param file database file path on the disk
 * param db_values target database array address
 * param cval database size
 */
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
    rc = sqlite3_exec(db, "SELECT count(distinct segmentid) FROM measuredvalue;", cb_size, cval, &zErrMsg);
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
        sprintf((char *) &query, "SELECT count() FROM measuredvalue WHERE segmentid==%d GROUP BY segmentid;", i + 1);
        sqlite3_exec(db, (char *) query, cb_size, &query_size, NULL);

        db_private[i].vals = (mvalue *) malloc(query_size * sizeof(mvalue));
        db_private[i].cvals = 0;

        sprintf((char *) &query, "SELECT measuredat, MAX(blood), MAX(ist) FROM measuredvalue WHERE segmentid==%d GROUP BY measuredat ORDER BY measuredat;", i + 1);
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
