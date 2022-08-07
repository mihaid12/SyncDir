
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_utile.h"


//
// IsSymbolicLinkValid
//
SDSTATUS
IsSymbolicLinkValid(
    __in char           *FileFullPath,
    __in char           *MainDirFullPath,    
    __out BOOL          *IsSymLinkValid,
    __out_opt char      *FileRealRelativePath
    )
/*++
Description: The routine validates a given file path by resolving all the symbolic links inside the path and telling whether or not
the file's real path points inside a given directory.
Concretely: the path FileFullPath is validated if the file's real path points inside the directory MainDirFullPath. If so, the routine outputs 
the file relative path and IsSymLinkValid is set to TRUE.

- FileFullPath: Pointer to the string containing the file full path.
- MainDirFullPath: Pointer to the string containing the full path of the main directory.
- IsSymLinkValid: Pointer to where the routine outputs the result of the symbolic link validation. TRUE, if valid, FALSE, otherwise.
    The caller provides the storage space.
- FileRealRelativePath: Optional. If not NULL, then the routine outputs at this address the real relative path of the file, in case 
it has passed the symbolic link validation. This is the path relative to the main directory MainDirFullPath, with all sub-paths resolved.
Storage space is provided by the caller: at least SD_MAX_PATH_LENGTH bytes.

E.g. let MainDirFullPath be "/home/user/main/" and FileFullPath be "/home/user/main/subdir/file" with no symlinks contained.
    Then the FileRealRelativePath will be "./subdir/file". On the other hand, supposing that subdir/file is a symbolic link 
    towards subdir/subdir_2/file_2, then FileRealRelativePath becomes "./subdir/subdir_2/file_2".

Return value: STATUS_SUCCESS, on success. STATUS_FAIL otherwise.
--*/
{
    SDSTATUS    status;
    char        fileRealFullPath[SD_MAX_PATH_LENGTH];

    // PREINIT.

    status = STATUS_FAIL;
    fileRealFullPath[0] = 0;

    // Parameter validation.

    if (NULL == FileFullPath || 0 == FileFullPath[0])
    {
        printf("[SyncDir] Error: IsSymbolicLinkValid(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == MainDirFullPath || 0 == MainDirFullPath[0])
    {
        printf("[SyncDir] Error: IsSymbolicLinkValid(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == IsSymLinkValid)
    {
        printf("[SyncDir] Error: IsSymbolicLinkValid(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }


    // INIT.
    // --

    
    // Main processing:

    // Get the full real path of the file (with all symbolic links resolved).

    if (NULL == realpath(FileFullPath, fileRealFullPath))
    {
        perror("[SyncDir] Error: IsSymbolicLinkValid(): realpath() failed.\n");
        status = STATUS_FAIL;
        goto cleanup_IsSymbolicLinkValid;
    }

    // Symlink points inside the main directory ? Verify:
    // If so, then output just the real relative file path.

    if (0 == strncmp(fileRealFullPath, MainDirFullPath, strlen(MainDirFullPath)))
    {
        (*IsSymLinkValid) = TRUE;

        if (NULL != FileRealRelativePath)
        {
            sprintf(FileRealRelativePath, ".%s", fileRealFullPath + strlen(MainDirFullPath) );   // Forming "./relative_path".
        }
    }
    else
    {
        printf("[SyncDir] Warning: IsSymbolicLinkValid(): Link points outside of the main directory [%s].\n", fileRealFullPath);
        (*IsSymLinkValid) = FALSE;

        if (NULL != FileRealRelativePath)
        {
            strcpy(FileRealRelativePath, fileRealFullPath);
        }
    }


    // If here, everything is ok.
    status = STATUS_SUCCESS;


    // UNINIT. Cleanup.
    cleanup_IsSymbolicLinkValid:

    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Output NULL, on fail.
        if (NULL != FileRealRelativePath)
        {
            FileRealRelativePath[0] = 0;
        }
    }

    return status;
} // IsSymbolicLinkValid()




//
// IsDirectoryValid
//
SDSTATUS
IsDirectoryValid(
    __in char       *DirPath,
    __out BOOL      *IsValid
    )
/*++
Description: The routine verifies if DirPath is a valid directory path, i.e. if the directory can be accessed (executed). 

- DirPath: Pointer to the string containing the directory path.
- IsValid: Pointer to where the routine outputs the result. TRUE, for a valid directory. FALSE, otherwise.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    DIR *dirStream;

    // PREINIT.

    status = STATUS_FAIL;
    dirStream = NULL;

    // Parameter validation.

    if (NULL == DirPath)
    {
        printf("[SyncDir] Error: IsDirectoryValid(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    } 
    if (NULL == IsValid)
    {
        printf("[SyncDir] Error: IsDirectoryValid(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }


    // INIT.
    // --
    
    // Main processing:

    // Try to access the directory with opendir(). Verify the result.

    dirStream = opendir(DirPath);
    if(NULL == dirStream)
    {
        perror("[SyncDir] Error: IsDirectoryValid(): The provided directory does not exist or it cannot be accessed.\n");
        (*IsValid) = FALSE;
    }
    else
    {
        // Directory exists and can be accessed.
        (*IsValid) = TRUE;
    }


    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    //cleanup_IsDirectoryValid:

    if (SUCCESS(status))
    {
        if (NULL != dirStream)
        {
            closedir(dirStream);
            dirStream = NULL;
        }
    }
    else
    {
        if (NULL != dirStream)
        {
            closedir(dirStream);
            dirStream = NULL;
        }        
    }

    return status;
} // IsDirectoryValid()




//
// IsPathSymbolicLink
//
SDSTATUS
IsPathSymbolicLink(
    __in char       *FileFullPath,
    __out BOOL      *IsSymLink
    )
/*++
    Description: The routine checks whether a given file path is soft link or not.

    - FileFullPath: Pointer to the string containing the path of the file to be checked.
    - IsSymLink: Pointer to where the routine outputs the result. TRUE, if FileFullPath is soft link. FALSE, otherwise.

    Return value: STATUS_SUCCESS, on success. STATUS_FAIL, otherwise.
--*/
{
    SDSTATUS status;
    struct stat fileStat;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == FileFullPath || 0 == FileFullPath[0])
    {
        printf("[SyncDir] Error: IsPathSymbolicLink(): Invalid parameter 1.\n");
        return STATUS_FAIL;        
    }
    if (NULL == IsSymLink)
    {
        printf("[SyncDir] Error: IsPathSymbolicLink(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }



    // INIT.
    // --


    // Main processing.

    // Get file information.
    if (lstat(FileFullPath, &fileStat) < 0)
    {
        perror("[SyncDir] Error: IsPathSymbolicLink(): Could not execute lstat(). \n");
        printf("lstat() error was for file [%s]. ", FileFullPath);
        status = STATUS_FAIL;
        goto cleanup_IsPathSymbolicLink;
    }

    // Check for symbolic link.
    if (S_ISLNK(fileStat.st_mode))
    {
        (*IsSymLink) = TRUE;
    }        
    else
    {
        (*IsSymLink) = FALSE;
    }


    // If here, everything worked well.
    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    cleanup_IsPathSymbolicLink:

    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // IsPathSymbolicLink



//
// IsFileValid
//
BOOL
IsFileValid(
    __in char *FileFullPath
    )
/*++
Description: The routine verifies if a file exists or not.
- FileFullPath: Pointer to the complete (absolute) path of the file.
Return value: TRUE if the file exists, FALSE otherwise.
--*/
{
    struct stat fileStat;
    
    if (lstat(FileFullPath, &fileStat) < 0)
    {
        return FALSE;
        // File does not exist, or "execute" permission is not granted for a subpath in the FileFullPath.
    }

    return TRUE;
    // File exists. (Though permissions are unknown for the file access.)

} // IsFileValid()



//
// MD5HashOfFile
//
SDSTATUS
MD5HashOfFile(
    __in char   *FileFullPath,
    __out char  *MD5Hash
    )
/*++
    Description: The routine outputs the MD5 hash for a given file content. The hash is output at the address pointed by MD5Hash.
    The file is identified by FileFullPath.

    - FileFullPath: Pointer to the full path of the file.
    - MD5Hash: Pointer containing the address where the MD5 hash of the file is output. Storage must be caller provided.
    
    Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS    status;
    FILE        *commandPipe;
    char        commandBuffer[SD_MAX_PATH_LENGTH + 30];
    char        md5HashBuffer[SD_HASH_CODE_LENGTH + 1]; 

    // PREINIT.

    status = STATUS_FAIL;
    commandPipe = NULL;
    commandBuffer[0] = 0;
    md5HashBuffer[0] = 0;

    // Parameter validation.

    if (NULL == FileFullPath)
    {
        perror("[SyncDir] Error: MD5HashOfFile(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (!IsFileValid(FileFullPath))
    {
        perror("[SyncDir] Error: MD5HashOfFile(): Invalid parameter 1. (File does not exist.)\n");
        return STATUS_FAIL;
    }
    if (NULL == MD5Hash)
    {
        perror("[SyncDir] Error: MD5HashOfFile(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }


    //
    // INIT.
    //


    // Initialize the command.
    // Use awk to print the first "word" (md5sum returns 32 chars + space + md5sum flag + file name).

    sprintf(commandBuffer, "md5sum \"%s\" | awk '{print $1}'", FileFullPath);


    // Initialize the command output pipe, to get the results.

    commandPipe = popen(commandBuffer, "r");
    if (NULL == commandPipe)
    {
        perror("[SyncDir] Error: MD5HashOfFile(): Error at popen().\n");
        status = STATUS_FAIL;
        goto cleanup_MD5HashOfFile;
    }



    //
    // Main processing.
    //


    // Get the MD5 hash of the file.

    if (NULL == fgets(md5HashBuffer, sizeof(md5HashBuffer), commandPipe))
    {
        perror("[SyncDir] Warning: MD5HashOfFile(): EOF or error at fgets().\n");
    }


    // Output the MD5 hash.

    strcpy(MD5Hash, md5HashBuffer);



    // If here, everything worked well.
    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    cleanup_MD5HashOfFile:

    if (SUCCESS(status))
    {
        // Close pipe.
        if (NULL != commandPipe)
        {
            if (-1 == pclose(commandPipe))
            {
                printf("[SyncDir] Warning: MD5HashOfFile(): Pipe close may not have succeeded.\n");
            }
            commandPipe = NULL;
        }
    }
    else
    {
        // Close pipe.
        if (NULL != commandPipe)
        {
            if (-1 == pclose(commandPipe))
            {
                printf("[SyncDir] Warning: MD5HashOfFile(): Pipe close may not have succeeded.\n");
            }
            commandPipe = NULL;
        }

        // Output NULL, on fail.
        MD5Hash[0] = 0;
    }

    return status;
} // MD5HashOfFile




//
// ExecuteShellCommand
//
SDSTATUS
ExecuteShellCommand(
    __in char           *ShellCommand,
    __out_opt char      *CommandOutput,
    __in_opt DWORD      OutputSizeIn
    )
/*++
Description: The routine executes a shell command and outputs the result. 
The command pointed by ShellCommand is executed and its output is stored at the address in CommandOutput (if not NULL), with an
allocated space of OutputSizeIn bytes.

- ShellCommand: Pointer to the string containing the command to be executed in shell.
- CommandOutput: Optional. Pointer to where the routine stores the execution output. Storage must be provided by the caller.
- OutputSizeIn: Optional. The size of the CommandOutput buffer, in bytes. Used only if CommandOutput is not NULL.

Return value: STATUS_SUCCESS on success. STATUS_FAIL otherwise.
--*/
{
    SDSTATUS        status;
    char            *outputBuffer;
    DWORD           outBufferLength;
    FILE            *commandPipe;

    // PREINIT.

    status = STATUS_FAIL;
    outputBuffer = NULL;
    outBufferLength = 0;
    commandPipe = NULL;

    // Parameter validation.

    if (NULL == ShellCommand || 0 == ShellCommand[0])
    {
        printf("[SyncDir] Error: ExecuteShellCommand(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }
    if (NULL != CommandOutput && 0 == OutputSizeIn)
    {
        printf("[SyncDir] Error: ExecuteShellCommand(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }


    // INIT.
    // --


    //printf("*** ExecuteShellCommand(): Exiting on purpose. No command executed.\n");
    //return STATUS_SUCCESS;


    //
    // Main processing:
    //


    // Allocate space for the command output.

    if (NULL != CommandOutput)
    {
        outBufferLength = OutputSizeIn;            
    }
    else
    {
        outBufferLength = 100;
    }

    outputBuffer = (char*) malloc(outBufferLength * sizeof(char));
    if (NULL == outputBuffer)
    {
        perror("[SyncDir] Error: ExecuteShellCommand(): Error at malloc().\n");
        status = STATUS_FAIL;
        goto cleanup_ExecuteShellCommand;
    }


    // Execute the command and open a pipe on reading, for getting the results.

    commandPipe = popen(ShellCommand, "r");
    if (NULL == commandPipe)
    {
        perror("[SyncDir] Error: ExecuteShellCommand(): Error at popen().\n");
        status = STATUS_FAIL;
        goto cleanup_ExecuteShellCommand;
    }


    // Get the command execution output.

    if (NULL == fgets(outputBuffer, outBufferLength, commandPipe))
    {
        printf("[SyncDir] Info: Shell command executed. Command output: [NULL].\n");
    }
    else
    {
        printf("[SyncDir] Info: Shell command executed. Command output: [%s].\n", outputBuffer);
    }


    // If here, everything is ok.
    // Output the results.

    if (NULL != CommandOutput && 0 != OutputSizeIn)
    {
        strncpy(CommandOutput, outputBuffer, OutputSizeIn);
        CommandOutput[OutputSizeIn - 1] = 0;
    }


    status = STATUS_SUCCESS;


    // UNINIT. Cleanup.
    cleanup_ExecuteShellCommand:

    if (SUCCESS(status))
    {
        // Close pipe.
        if (NULL != commandPipe)
        {
            if (-1 == pclose(commandPipe))
            {
                printf("[SyncDir] Warning: ExecuteShellCommand(): Pipe close may not have succeeded.\n");
            }
            commandPipe = NULL;
        }

        // Free buffer space.
        if (NULL != outputBuffer)
        {
            free(outputBuffer);
            outputBuffer = NULL;
        }
    }
    else
    {
        // Close pipe.
        if (NULL != commandPipe)
        {
            if (-1 == pclose(commandPipe))
            {
                printf("[SyncDir] Warning: ExecuteShellCommand(): Pipe close may not have succeeded.\n");
            }
            commandPipe = NULL;
        }

        // Free buffer space.
        if (NULL != outputBuffer)
        {
            free(outputBuffer);
            outputBuffer = NULL;
        }

        // Output NULL, on fail.
        if (NULL != CommandOutput && 0 != OutputSizeIn)
        {
            CommandOutput[0] = 0;
        }
    }

    return status;
} // ExecuteShellCommand()


