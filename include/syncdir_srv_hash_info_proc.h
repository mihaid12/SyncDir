
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_SRV_HASH_INFO_PROC_H_
#define _SYNCDIR_SRV_HASH_INFO_PROC_H_



#include "syncdir_srv_def_types.h"



//
// Interfaces:
//


//
// UpdateOrDeleteHashInfosForDirPath
//
SDSTATUS
UpdateOrDeleteHashInfosForDirPath(
    __in char       *DirFullPath,
    __in char       *DirRelativePath,
    __in_opt char   *NewDirRelativePath,
    __in const char *UpdateOrDelete,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    );
/*++
Description: 
    The routine updates or deletes all the HASH_INFO structures of the files inside a given direcory and all of its
    subdirectories (recursively). The directory is indicated by the DirFullPath, DirRelativePath and NewDirRelativePath arguments. The latter
    is used only if an update is indicated by the UpdateOrDelete argument. In this case the file paths inside the HashInfo's are updated
    to include the new path NewDirRelativePath. The behaviour of the routine (update/delete) is dictated by the UpdateOrDelete argument.
Arguments:
    - DirFullPath: Pointer to the directory full path.
    - DirRelativePath: Pointer to the directory relative path (the path is relative to the main directory of the application).
    - NewDirRelativePath: Pointer to the new relative path of the directory (used in case UpdateOrDelete is set to "UPDATE").
        This argument can be NULL, if UpdateOrDelete is set to "DELETE", since it is not used.
    - UpdateOrDelete: Pointer to a string indicating the routine behaviour. "UPDATE" and "DELETE" are the possible values.
    - HahsInfoHMap: Reference to the map containing all the HASH_INFO structures of all the files on the server.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// UpdateHashInfoOfNondirFile
//
SDSTATUS
UpdateHashInfoOfNondirFile(
    __in char *FileRelativePath,
    __in char *NewFileRelativePath,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    );
/*++
Description: 
    The routine updates the HASH_INFO structure of the file at path FileRelativePath. The file's HashInfo is updated with the new
    file path NewFileRelativePath and made accessibile by the same new path.
Arguments:
    - FileRelativePath: Pointer to the file relative path.
    - NewFileRelativePath: Pointer to the new relative path of the file.
    - HahsInfoHMap: Reference to the map containing all the HASH_INFO structures of all the files on the server.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).

--*/



//
// DeleteHashInfoOfFile
//
SDSTATUS
DeleteHashInfoOfFile(
    __in const char *FileRelativePath,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    );
/*++
Description: 
    The routine deletes from HashInfoHMap the HASH_INFO structure corresponding to the file FileRelativePath.
Arguments:
    - FileRelativePath: Pointer to the relative path of the file (the path is relative to the main directory of the application).
    - HahsInfoHMap: Reference to the map containing the HASH_INFO structures.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// InsertHashInfoOfFile
//
SDSTATUS
InsertHashInfoOfFile(
    __in const char   *FileRelativePath,
    __in const char   *HashCode,
    __in DWORD  FileSize,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    );
/*++
Description: 
    The routine inserts in the map HashInfoHMap a HASH_INFO structure corresponding to the file at path FileRelativePath.
    The new HASH_INFO contains the hash code HashCode and size FileSize.
Arguments:
    - FileRelativePath: Pointer to the file relative path.
    - HashCode: Pointer to the hash code of the file.
    - FileSize: The size of the file, in bytes.
    - HahsInfoHMap: Reference to the map where the HASH_INFO structures are stored. 
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// BuildHashInfoForEachFile
//
extern "C"                                                                  // Need it in syncdir_srv_main.c, so make it callable by C compiler.
SDSTATUS
BuildHashInfoForEachFile(
    __in const char    *DirFullPath,
    __in const char    *DirRelativePath,
    __out std::unordered_map<std::string, HASH_INFO> & HashInfoHMap    
    );
/*++
Description: 
    The routine builds the map containing all the HASH_INFO structures of every file inside the DirFullPath directory and its
    subdirectories. These structures are made accessible through the hash code of the file (e.g. using MD5 algorithm) and thorugh the file 
    relative path.
Arguments:
    - DirFullPath: Reference to the string containing the full path of the directory.
    - DirRelativePath: Reference to the string containing the relative path of the directory.
    - HahsInfoHMap: Reference to the map where the routine stores all the HASH_INFO structures.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



#endif // _SYNCDIR_SRV_HASH_INFO_PROC_H_

