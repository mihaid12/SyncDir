
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_ESSENTIAL_DEF_TYPES_H_
#define _SYNCDIR_ESSENTIAL_DEF_TYPES_H_
/*++
Header containing the essential SyncDir structure types and definitions. Required for both client and server.
--*/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>

#ifdef __cplusplus
    #include "SyncDirException.h"
    #include <iostream>
    #include <string>
    #include <unordered_map>
    
    using namespace std;
#endif



#define __in 
#define __out
#define __inout
#define __in_opt
#define __out_opt
#define __inout_opt

#define SD_STDLOG stdout                                                        // SyncDir standard log file stream (e.g. stdout).
#define SD_STDLOG_FD 1
//#define SD_STDLOG(fs) fs
//  --> usage: SD_STDLOG(gLog), for a global file stream.

#define SD_PACKET_DATA_SIZE 1024
#define SD_MAX_PATH_LENGTH 4096
#define SD_MAX_FILENAME_LENGTH 256

#define SD_SHORT_MSG_SIZE 20
#define SD_HASH_CODE_LENGTH 32

#define SUCCESS(x) (0 <= x)
#define SUCCESS_KEEP_WARNING(x) (SUCCESS(x) ? x : STATUS_SUCCESS)               // Returns a success status. Includes warning, if case.
//#define STATUS_SUCCESS 0
//#define STATUS_FAIL -1
//#define TRUE 1
//#define FALSE 0
#define SD_MIN(a,b) (a <= b ? a : b)



typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef int8_t __int8;
typedef int16_t __int16;
typedef int32_t __int32;
typedef int64_t __int64;
typedef enum { FALSE = 0, TRUE = 1 } BOOL;
typedef enum { STATUS_FAIL = -1, STATUS_SUCCESS = 0, STATUS_WARNING = 1} SDSTATUS;



//
// OP_TYPE - The general type of operations that are treated.
//
typedef enum _OP_TYPE
{
    opDIRCREATE,              // DIR - Directory, FIL - Other files (regular, sym/hard links, etc.)
    opFILCREATE,
    opCREATE,
    opDIRDELETE,
    opFILDELETE,
    opDELETE,
    opDIRMOVEDFROM,
    opFILMOVEDFROM,    
    opMOVEDFROM,
    opDIRMOVEDTO,
    opFILMOVEDTO,
    opMOVEDTO,
    opDIRMOVE,
    opFILMOVE,
    opMOVE,
    opMODIFY,
    opUNKNOWN
} OP_TYPE;



//
// FILE_TYPE - Differentiating between different file types.
//
typedef enum _FILE_TYPE
{
    ftREGULAR,
    ftDIRECTORY,
    ftSYMLINK,
    ftHARDLINK,
    ftNONDIR,        // non-Directory file
    ftUNKNOWN
} FILE_TYPE;



//
// PACKET_OP - Main operation packet, which can include any type of operation.
//
typedef struct _PACKET_OP
{
    OP_TYPE     OperationType;                                                  // Type of operation.
    FILE_TYPE   FileType;                                                       // Type of file.
    WORD        RelativePathLength;                                             // Length of file's relative path.
    WORD        RealRelativePathLength;                                         // Length of file's real relative path (for ftSYMLINK only).
    WORD        OldRelativePathLength;                                          // Length of file's old relative path (for MOVE operations).
    // char MD5Hash[32+1];    
} PACKET_OP, *PPACKET_OP;



//
// PACKET_FILE - Packet used for transfering file content chunks.
//
typedef struct _PACKET_FILE
{
    BOOL    IsEOF;                                                              // Flag indicating if EOF was met.
    DWORD   ChunkSize;                                                          // Size of the FileChunk data (from 0 to SD_PACKET_DATA_SIZE).
    char    FileChunk[SD_PACKET_DATA_SIZE];                                     // File content.
} PACKET_FILE, *PPACKET_FILE;



//
// Structures for Individual Operations:
//


//
// PACKET_OP_DELETE
//
typedef struct _PACKET_OP_DELETE
{
    FILE_TYPE FileType;
    WORD PathLength;    // Including '\0'.
} PACKET_OP_DELETE, *PPACKET_OP_DELETE;


//  
// PACKET_OP_MOVE
// 
typedef struct _PACKET_OP_MOVE
{
    FILE_TYPE FileType;
    char OldFilePath[SD_MAX_PATH_LENGTH];
    char NewFilePath[SD_MAX_PATH_LENGTH];
} PACKET_OP_MOVE, *PPACKET_OP_MOVE;


//
// PACKET_OP_MODIFY
//
typedef struct _PACKET_OP_MODIFY
{
    FILE_TYPE FileType;
    WORD PathLength;    // Including '\0'.
    char MD5Hash[32+1];
} PACKET_OP_MODIFY, *PPACKET_OP_MODIFY;


//
// PACKET_OP_CREATE
//
typedef struct _PACKET_OP_CREATE
{
    FILE_TYPE FileType;
    char RelativePath[SD_MAX_PATH_LENGTH];    
    char RealRelativePath[SD_MAX_PATH_LENGTH];                                          // Used only if FileType is ftSYMLINK.
} PACLET_OP_CREATE, *PPACKET_OP_CREATE;
  








#endif //--> #ifndef _SYNCDIR_ESSENTIAL_DEF_TYPES_H_

