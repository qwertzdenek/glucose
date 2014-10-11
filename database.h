/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
database.h
*/

#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include "structures.h"

int init_data(char *file, mvalue_ptr **database_values, int *cval);

#endif // DATABASE_H_INCLUDED
