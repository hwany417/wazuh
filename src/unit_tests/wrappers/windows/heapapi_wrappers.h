
/*
 * Copyright (C) 2015-2020, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#ifndef HEAPAPI_H
#define HEAPAPI_H

#include <windows.h>

LPVOID wrap_win_alloc(SIZE_T size);

#endif
