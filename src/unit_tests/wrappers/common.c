/* Copyright (C) 2015-2020, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "common.h"
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "headers/defs.h"

time_t time_mock_value;
#ifdef WIN32
int test_mode = 0;
#endif

int FOREVER() {
    return 1;
}

int __wrap_FOREVER() {
    return mock();
}

time_t wrap_time (__UNUSED_PARAM(time_t *t)) {
    return time_mock_value;
}

int wrap_fprintf(FILE *__stream, const char *__format, ...) {
    int ret;
    char formatted_msg[OS_MAXSTR];
    va_list args;
    printf("%s", __stream);

    va_start(args, _Format);
    if (test_mode) {
        vsnprintf(formatted_msg, OS_MAXSTR, _Format, args);
        check_expected(__stream);
        check_expected(formatted_msg);
    } else {
        ret = fprintf(__stream, _Format, args);
    }

    va_end(args);
    if(test_mode) {
        return mock();
    }
    return ret;
}

char * wrap_fgets (char * __s, int __n, FILE * __stream) {
  char *buffer = mock_type(char*);

  check_expected(__stream);

  if(buffer) {
    strncpy(__s, buffer, __n - 1);
    return __s;
  }
  return NULL;
}
