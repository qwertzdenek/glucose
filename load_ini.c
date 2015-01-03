/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
load_ini.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "load_ini.h"
#include "structures.h"

struct symbol
{
    char name[16];
    float val;
};

/**
 * Converts string to lowercase
 */
char *lower(char *s)
{
    char *ptr = s;

    while ((*ptr = tolower(*ptr)) != 0)
        ptr++;

    return s;
}

int next_var(FILE *fp, struct symbol *sym)
{
    const int MAX_LINE = 24;
    char tmp[MAX_LINE];
    char ctest;
    char *ptr = sym->name;

    while (isspace(ctest = fgetc(fp)))
            ;

    fseek(fp, -1, SEEK_CUR);

    if (ctest == EOF)
        return 0;

    if (ctest == ';')
        while ((ctest = fgetc(fp)) != 10)
            ;

    // read name
    while (((*ptr = (char) fgetc(fp)) != '=') && (ptr < (sym->name + MAX_LINE)))
        ptr++;
    *ptr = 0;

    // read value
    ptr = (char *) &tmp;
    while (!isspace(*ptr = fgetc(fp)))
        ptr++;
    *ptr = 0;

    sym->val = strtof((char *) &tmp, &ptr);

    return 1;
}

int read_bounds(FILE *fp, bounds *bconf)
{
    struct symbol sym;

    while (next_var(fp, &sym))
    {
        if (!strcmp(sym.name, "pmin"))
            bconf->pmin = sym.val;
        else if (!strcmp(sym.name, "pmax"))
            bconf->pmax = sym.val;
        else if (!strcmp(sym.name, "cgmin"))
            bconf->cgmin = sym.val;
        else if (!strcmp(sym.name, "cgmax"))
            bconf->cgmax = sym.val;
        else if (!strcmp(sym.name, "cmin"))
            bconf->cmin = sym.val;
        else if (!strcmp(sym.name, "cmax"))
            bconf->cmax = sym.val;
        else if (!strcmp(sym.name, "ppmin"))
            bconf->ppmin = sym.val;
        else if (!strcmp(sym.name, "ppmax"))
            bconf->ppmax = sym.val;
        else if (!strcmp(sym.name, "cgpmin"))
            bconf->cgpmin = sym.val;
        else if (!strcmp(sym.name, "cgpmax"))
            bconf->cgpmax = sym.val;
        else if (!strcmp(sym.name, "cpmin"))
            bconf->cpmin = sym.val;
        else if (!strcmp(sym.name, "cpmax"))
            bconf->cpmax = sym.val;
        else if (!strcmp(sym.name, "dtmin"))
            bconf->dtmin = sym.val;
        else if (!strcmp(sym.name, "dtmax"))
            bconf->dtmax = sym.val;
        else if (!strcmp(sym.name, "hmin"))
            bconf->hmin = sym.val;
        else if (!strcmp(sym.name, "hmax"))
            bconf->hmax = sym.val;
        else if (!strcmp(sym.name, "kmin"))
            bconf->kmin = sym.val;
        else if (!strcmp(sym.name, "kmax"))
            bconf->kmax = sym.val;
        else if (!strcmp(sym.name, "mmin"))
            bconf->mmin = sym.val;
        else if (!strcmp(sym.name, "mmax"))
            bconf->mmax = sym.val;
        else if (!strcmp(sym.name, "nmin"))
            bconf->nmin = sym.val;
        else if (!strcmp(sym.name, "nmax"))
            bconf->nmax = sym.val;
    }

    return 0;
}

int load_ini(char *file, bounds *bconf)
{
    char buf[12];
    char *ptr;

    // TODO: loading
    FILE *fp = fopen(file, "r");

    // read first symbol
    ptr = (char *) &buf;
    while (!isspace(*ptr = fgetc(fp)))
        ptr++;
    *ptr = 0;

    // compare with initial symbol
    if (strcmp(lower(buf), "[bounds]"))
        return -1;
    else // found
        read_bounds(fp, bconf);

    fclose(fp);
    return 0;
}
