
/*
 * Copyright (C) 2015-2020, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#ifndef STRINGAPISET_H
#define STRINGAPISET_H

#include <windows.h>

int wrap_WideCharToMultiByte(UINT CodePage,
                             DWORD dwFlags,
                             LPCWCH lpWideCharStr,
                             int cchWideChar,
                             LPSTR lpMultiByteStr,
                             int cbMultiByte,
                             LPCCH lpDefaultChar,
                             LPBOOL lpUsedDefaultChar);


#endif
