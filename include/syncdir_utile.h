
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_UTILE_H_
#define _SYNCDIR_UTILE_H_
/*++
Header of the source file providing useful routines, for diverse SyncDir tasks.
--*/



#include "syncdir_essential_def_types.h"



#ifdef __cplusplus                  // Because it contains functions written in C. So let C++ compiler know how to call these.
extern "C"                          // Need "ifdef __cplusplus", since C compiler does not recognize extern "C".
{
#endif


//
// Interfaces:
//


//
// IsSymbolicLinkValid
//
SDSTATUS
IsSymbolicLinkValid(
    __in char           *FileFullPath,
    __in char           *MainDirFullPath,    
    __out BOOL          *IsSymLinkValid,
    __out_opt char      *FileRealRelativePath
    );
/*++
Description: 
    The routine validates a given file path by resolving all the symbolic links inside the path and telling whether or not
    the file's real path points inside a given directory.
    Concretely: the path FileFullPath is validated if the file's real path points inside the directory MainDirFullPath. If so, the routine 
    outputs the file relative path and IsSymLinkValid is set to TRUE.
Arguments:
    - FileFullPath: Pointer to the string containing the file full path.
    - MainDirFullPath: Pointer to the string containing the full path of the main directory.
    - IsSymLinkValid: Pointer to where the routine outputs the result of the symbolic link validation. TRUE, if valid, FALSE, otherwise.
        The caller provides the storage space.
    - FileRealRelativePath: Optional. If not NULL, then the routine outputs at this address the real relative path of the file, in case 
    it has passed the symbolic link validation. This is the path relative to the main directory MainDirFullPath, with all sub-paths resolved.
    Storage space is provided by the caller: at least SD_MAX_PATH_LENGTH bytes.
E.g.:
    Let MainDirFullPath be "/home/user/main/" and FileFullPath be "/home/user/main/subdir/file" with no symlinks contained.
        Then the FileRealRelativePath will be "./subdir/file". On the other hand, supposing that subdir/file is a symbolic link 
        towards subdir/subdir_2/file_2, then FileRealRelativePath becomes "./subdir/subdir_2/file_2".
Return value: 
    STATUS_SUCCESS, on success. STATUS_FAIL otherwise.
--*/




//
// IsDirectoryValid
//
SDSTATUS
IsDirectoryValid(
    __in char       *DirPath,
    __out BOOL      *IsValid
    );
/*++
Description: 
    The routine verifies if DirPath is a valid directory path, i.e. if the directory can be accessed (executed). 
Arguments:
    - DirPath: Pointer to the string containing the directory path.
    - IsValid: Pointer to where the routine outputs the result. TRUE, for a valid directory. FALSE, otherwise.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/




//
// IsPathSymbolicLink
//
SDSTATUS
IsPathSymbolicLink(
    __in char       *FileFullPath,
    __out BOOL      *IsSymLink
    );
/*++
Description: 
    The routine checks whether a given file path is soft link or not.
Arguments:
    - FileFullPath: Pointer to the string containing the path of the file to be checked.
    - IsSymLink: Pointer to where the routine outputs the result. TRUE, if FileFullPath is soft link. FALSE, otherwise.
Return value: 
    STATUS_SUCCESS, on success. STATUS_FAIL, otherwise.
--*/




//
// IsFileValid
//
BOOL
IsFileValid(
    __in char *FileFullPath
    );
/*++
Description: 
    The routine verifies if a file exists or not.
Arguments:
    - FileFullPath: Pointer to the complete (absolute) path of the file.
Return value: 
    TRUE if the file exists, FALSE otherwise.
--*/



//
// MD5HashOfFile
//
SDSTATUS
MD5HashOfFile(
    __in char   *FileFullPath,
    __out char  *MD5Hash
    );
/*++
Description: 
    The routine outputs the MD5 hash for a given file content. The hash is output at the address pointed by MD5Hash.
    The file is identified by FileFullPath.
Arguments:
    - FileFullPath: Pointer to the full path of the file.
    - MD5Hash: Pointer containing the address where the MD5 hash of the file is output.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/




//
// ExecuteShellCommand
//
SDSTATUS
ExecuteShellCommand(
    __in char           *ShellCommand,
    __out_opt char      *CommandOutput,
    __in_opt DWORD      OutputSizeIn
    );
/*++
Description: 
    The routine executes a shell command and outputs the result. 
    The command pointed by ShellCommand is executed and its output is stored at the address in CommandOutput (if not NULL), with an
    allocated space of OutputSizeIn bytes.
Arguments:
    - ShellCommand: Pointer to the string containing the command to be executed in shell.
    - CommandOutput: Optional. Pointer to where the routine stores the execution output. Storage must be provided by the caller.
    - OutputSizeIn: Optional. The size of the CommandOutput buffer, in bytes. Used only if CommandOutput is not NULL.
Return value: 
    STATUS_SUCCESS on success. STATUS_FAIL otherwise.
--*/



#ifdef __cplusplus
}
#endif





#endif //--> #ifndef _SYNCDIR_UTILE_H_

