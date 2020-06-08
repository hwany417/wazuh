/* Copyright (C) 2015-2020, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <stdio.h>

extern int test_mode;

int FOREVER();

int __wrap_FOREVER();

int wrap_fprintf (FILE *__stream, const char *__format, ...);

char * wrap_fgets (char * __s, int __n, FILE * __stream);

time_t wrap_time (time_t *t);

#ifdef WIN32
extern time_t time_mock_value;

#define time(x) wrap_time(x)
#endif

#endif /* COMMON_H */

