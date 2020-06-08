/* Copyright (C) 2015-2020, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#ifndef SECURITYBASEAPI_H
#define SECURITYBASEAPI_H

#include <windows.h>

WINBOOL wrap_IsValidSid (PSID pSid);

WINBOOL wrap_GetSecurityDescriptorDacl (PSECURITY_DESCRIPTOR pSecurityDescriptor, 
                                        LPBOOL lpbDaclPresent,
                                        PACL *pDacl,
                                        LPBOOL lpbDaclDefaulted);

WINBOOL wrap_GetAclInformation (PACL pAcl, 
                                LPVOID pAclInformation,
                                DWORD nAclInformationLength,
                                ACL_INFORMATION_CLASS dwAclInformationClass);

WINBOOL wrap_GetAce (PACL pAcl,
                     DWORD dwAceIndex,
                     LPVOID *pAce);

WINBOOL wrap_AdjustTokenPrivileges (HANDLE TokenHandle, 
                                    WINBOOL DisableAllPrivileges,
                                    PTOKEN_PRIVILEGES NewState,
                                    DWORD BufferLength,
                                    PTOKEN_PRIVILEGES PreviousState,
                                    PDWORD ReturnLength);
                                    
WINBOOL wrap_AllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                                      BYTE nSubAuthorityCount,
                                      DWORD nSubAuthority0,
                                      DWORD nSubAuthority1,
                                      DWORD nSubAuthority2,
                                      DWORD nSubAuthority3,
                                      DWORD nSubAuthority4,
                                      DWORD nSubAuthority5,
                                      DWORD nSubAuthority6,
                                      DWORD nSubAuthority7,
                                      PSID *pSid);

WINBOOL wrap_InitializeAcl(PACL pAcl,
                           DWORD nAclLength,
                           DWORD dwAclRevision);

WINBOOL wrap_CopySid(DWORD nDestinationSidLength,
                     PSID pDestinationSid,
                     PSID pSourceSid);

WINBOOL wrap_AddAce(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD dwStartingAceIndex,
                    LPVOID pAceList,
                    DWORD nAceListLength);

WINBOOL wrap_EqualSid(PSID pSid1,
                      PSID pSid2);

BOOL wrap_DeleteAce(PACL  pAcl,
                    DWORD dwAceIndex);

#endif
