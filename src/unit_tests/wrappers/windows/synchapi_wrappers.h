/* Copyright (C) 2015-2020, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#ifndef SYNCHAPI_H
#define SYNCHAPI_H

#include <windows.h>

VOID wrap_Sleep (DWORD dwMilliseconds);

HANDLE wrap_CreateEvent (LPSECURITY_ATTRIBUTES lpEventAttributes,
                         WINBOOL bManualReset,
                         WINBOOL bInitialState,
                         LPCSTR lpName);

#endif
