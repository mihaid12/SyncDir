
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_clt_main.h"


//__int32 gCltSock = -1;              // definition (+ initialize at neutral value)

QWORD gTimeLimit = 0;               // definition (initialize at 0 == infinity).

FILE *g_SD_STDLOG;                  // definition only.




//
// main
//
int main(int argc, char **argv)
{
    MainCltRoutine(argc, argv);

    return 0;
}




//
// MainCltRoutine
//
SDSTATUS MainCltRoutine(
    __in int MainArgc,
    __in char **MainArgv
    )
/*++
Description: The routine verifies and validates all command-line arguments of SyncDir launch (MainArgc and MainArgv arguments), then 
executes 2 routine calls: CltReturnConnectedSocket and CltMonitorPartition. The former creates a socket connection to the SyncDir server
and the latter launches the client partition monitoring, which also loops the server update procedures.

- MainArgc: Length of the MainArgv array, i.e. number of command-line arguments provided at SyncDir client startup. 
- MainArgv: Pointer to the array of command-line arguments (strings) provided at SyncDir client launch.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{

    SDSTATUS    status;
    __int32     cltSock;
    char        mainDirFullPath[SD_MAX_PATH_LENGTH];
    char        *srvIP;
    DWORD       srvPort;
    BOOL        isSymLink;
    BOOL        isDirValid;

    // PREINIT.

    status = STATUS_FAIL;
    cltSock = -1;
    mainDirFullPath[0] = 0;
    srvIP = NULL;
    srvPort = 0;
    isSymLink = TRUE;
    isDirValid = FALSE;

    // Parameter validation (start).

    if (MainArgc < 5)
    {
        printf("[SyncDir] Error: MainCltRoutine(): Invalid number of parameters. \
            Please provide <port> <IP x.x.x.x> <directory path> <monitor time (seconds)>.\n");
        return STATUS_FAIL;
    }

    // Validate port.
    if (atoi(MainArgv[1]) <= 0)
    {
        printf("[SyncDir] Error: MainCltRoutine(): The first parameter is not a port number.\n");
        return STATUS_FAIL;
    }
    if ( (atoi(MainArgv[1]) < 1024) || (65535 < atoi(MainArgv[1])) )
    {
        printf("[SyncDir] Error: MainCltRoutine(): Please provide port between 1024 and 65535.\n");
        return STATUS_FAIL;
    }        
    if ( (1024 <= atoi(MainArgv[1])) && (atoi(MainArgv[1]) <= 49151) )
    {
        printf("[SyncDir] Warning: MainCltRoutine(): Port is between 1024 and 49151. Recommended port: between 49152 and 65535. \
            Continuing execution ...\n");
        status = STATUS_WARNING;
    }    

    // Validate main directory.
    status = IsDirectoryValid(MainArgv[3], &isDirValid);
    if (!(SUCCESS(status)))    
    {
        printf("[SyncDir] Error: MainCltRoutine(): Failed to execute IsDirectoryValid().\n");
        return STATUS_FAIL;
    }    
    if (FALSE == isDirValid)
    {
        printf("[SyncDir] Error: MainCltRoutine(): The fourth parameter is not a valid directory path.\n");
        return STATUS_FAIL;
    }

    // Verify that the main directory is not a symbolic link.
    status = IsPathSymbolicLink(MainArgv[3], &isSymLink); 
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: MainCltRoutine(): Failed to execute IsPathSymbolicLink().\n");
        return STATUS_FAIL;
    }    
    if (TRUE == isSymLink)
    {
        perror("[SyncDir] Error: MainCltRoutine(): The provided directory is a symbolic link. Please provide another directory.\n");
        return STATUS_FAIL;
    }    

    // If atoll() returns 0, but not due to string "0", then log an error. (E.g. maybe obtained from an atoll("letters").)
    if ((atoll(MainArgv[4]) <= 0) && (0 != strcmp(MainArgv[4], "0")))
    {
        printf("[SyncDir] Error: MainCltRoutine(): The fifth parameter is not a valid number of seconds. Please provide a non-negative integer.\n");
        return STATUS_FAIL;
    }

    //
    //--> Parameter validation (end).



    //
    // INIT.
    //

    // Global log file stream. Init.
    g_SD_STDLOG = stdout;

    // Init port, IP, main directory, time limit.
    srvPort = atoi(MainArgv[1]);
    srvIP = MainArgv[2];

    // Get the full real path of the main directory (i.e. all the symbolic links subpaths are resolved).
    if (NULL == realpath(MainArgv[3], mainDirFullPath))
    {
        perror("[SyncDir] Error: MainCltRoutine(): Could not get the real path of the main directory.\n");
        status = STATUS_FAIL;
        goto cleanup_MainCltRoutine;
    }    
    
    // Global time limit.
    gTimeLimit = atoll(MainArgv[4]);
    if (0 == gTimeLimit)
    {
        gTimeLimit = (QWORD)(-1);       // "infinity"
    }
    printf("gTimeLimit: [%lu] \n", gTimeLimit);
    


    //
    // Start main processing.
    //



    // Obtain socket connection to the server.
    
    status = CltReturnConnectedSocket(&cltSock, srvPort, srvIP);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: MainCltRoutine(): Failed at CltReturnConnectedSocket.\n");
        status = STATUS_FAIL;
        goto cleanup_MainCltRoutine;
    }    



    // Monitor file system partition & Update the server.

    status = CltMonitorPartition(mainDirFullPath, cltSock);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: MainCltRoutine(): Failed to execute CltMonitorPartition().\n");
        status = STATUS_FAIL;
        goto cleanup_MainCltRoutine;
    }



    // If here, everything went well.
    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    cleanup_MainCltRoutine:

    if (SUCCESS(status))
    {
        // Close connection/socket.
        if (0 < cltSock)
        {
            close(cltSock);
            cltSock = -1;
        }
    }
    else
    {
        if (0 < cltSock)
        {
            close(cltSock);
            cltSock = -1;
        }
    }

    return status;
} // MainCltRoutine().


