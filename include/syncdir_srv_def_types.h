
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_SRV_DEF_TYPES_H_
#define _SYNCDIR_SRV_DEF_TYPES_H_



#include "syncdir_essential_def_types.h"

#ifdef __cplusplus                                                  // For C usage as well (for C++ only the extern "C" is needed).
extern "C" 
{
    #include "syncdir_utile.h"
}
#else
    #include "syncdir_utile.h"
#endif



#ifdef __cplusplus                                                  // Declaration only.
    extern "C" FILE *g_SD_STDLOG;                                   // C compiler does not recognize this.
#else
    extern FILE *g_SD_STDLOG;
#endif



#define SD_MAX_CONNECTIONS 1



// *********************** C++ only (start) ***********************
#ifdef __cplusplus                                                      // Because HASH_INFO uses C++ libraries.

//
// HASH_INFO - Hash code information. Stores the 
//
typedef struct _HASH_INFO
{
    std::string     HashCode;                                           // Hash sum of the file content. Length: SD_HASH_CODE_LENGTH + 1
    std::string     FileRelativePath;                                   // Relative path of the file (relative to SyncDir main directory).
    DWORD           FileSize;                                           // Size of the file (in bytes).
} HASH_INFO, *PHASH_INFO;

#endif //--> #ifdef __cplusplus
// *********************** C++ only (end) ***********************







#endif //--> #ifndef _SYNCDIR_SRV_DEF_TYPES_H_

