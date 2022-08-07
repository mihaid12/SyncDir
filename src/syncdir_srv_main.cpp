
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_srv_main.h"


FILE *g_SD_STDLOG;                              // definition only.




//
// main
//
int main(int argc, char **argv)
{
    MainSrvRoutine(argc, argv);

    return 0;
}



//
// MainSrvRoutine
//
SDSTATUS MainSrvRoutine(
    __in int MainArgc, 
    __in char **MainArgv
    )
/*++
Description: The routine verifies and validates all command-line arguments of SyncDir launch (MainArgc and MainArgv arguments), then 
builds all the HashInfo structures for all the existent files (on the server partition) and finally starts accepting a SyncDir client
connection to receive file updates.

- MainArgc: Length of the MainArgv array, i.e. number of command-line arguments provided at SyncDir server startup. 
- MainArgv: Pointer to the array of command-line arguments (strings) provided at SyncDir server launch.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS            status;
    QWORD               opCount;
    char                mainDirFullPath[SD_MAX_PATH_LENGTH];
    __int32             srvSock;
    __int32             sockConnID;
    DWORD               srvPort;
    BOOL                isSymLink;
    BOOL                isDirValid;
    socklen_t           lenCltAddr;
    struct sockaddr_in  cltAddr;                                                // Client address info.    
    std::unordered_map<std::string, HASH_INFO> hashInfoHMap;

    // PREINIT.
    
    status = STATUS_FAIL;
    opCount = 0;
    mainDirFullPath[0] = 0;
    srvSock = -1;
    sockConnID = -1;
    srvPort = 0;
    isSymLink = TRUE;
    isDirValid = FALSE;
    lenCltAddr = 0;
    
    // Validate parameters (start).
    
    if (MainArgc < 3)
    {
        printf("[SyncDir] Error: Invalid number of parameters. \
            Please provide <port> <directory path>.\n");
        return STATUS_FAIL;
    }

    // Validate port.
    if (0 == atoi(MainArgv[1]))
    {
        printf("[SyncDir] Error: First argument is not a port number.\n");
        return STATUS_FAIL;
    }
    if ( (atoi(MainArgv[1]) < 1024) || (65535 < atoi(MainArgv[1])) )
    {
        printf("[SyncDir] Error: MainSrvRoutine(): Please provide port between 1024 and 65535.\n");
        return STATUS_FAIL;
    }        
    if ( (1024 <= atoi(MainArgv[1])) && (atoi(MainArgv[1]) <= 49151) )
    {
        printf("[SyncDir] Warning: MainSrvRoutine(): Port is between 1024 and 49151. Recommended port: between 49152 and 65535. \
            Continuing execution ...\n");
        status = STATUS_WARNING;
    }        

    // Validate main directory.
    status = IsDirectoryValid(MainArgv[2], &isDirValid);
    if (!(SUCCESS(status)))    
    {
        printf("[SyncDir] Error: MainSrvRoutine(): Failed to execute IsDirectoryValid().\n");
        return STATUS_FAIL;
    }    
    if (FALSE == isDirValid)
    {
        printf("[SyncDir] Error: MainSrvRoutine(): The second parameter is not a valid directory path.\n");
        return STATUS_FAIL;
    }

    // Verify that the main directory is not a symbolic link.
    status = IsPathSymbolicLink(MainArgv[2], &isSymLink); 
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: MainSrvRoutine(): Failed to execute IsPathSymbolicLink().\n");
        return STATUS_FAIL;
    }    
    if (TRUE == isSymLink)
    {
        perror("[SyncDir] Error: MainSrvRoutine(): The provided directory is a symbolic link. Please provide another directory.\n");
        return STATUS_FAIL;
    }    

    //--> Validate parameters (end).



    __try
    {

        // INIT.

        // Global log file stream. Init.
        g_SD_STDLOG = stdout;

        // Initialize server port and client address length for connection acceptance.
        srvPort = atoi(MainArgv[1]);
        lenCltAddr = sizeof(cltAddr);

        // Get the full real path of the main directory (i.e. all the symbolic links subpaths are resolved).
        if (NULL == realpath(MainArgv[2], mainDirFullPath))              
        {
            perror("[SyncDir] Error: MainSrvRoutine(): Could not get the real path of the main directory.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }



        //
        // Start main processing.
        //



        // Build the HashInfo's of all files on the server and store them in a hash map (hashInfoHMap).

        status = BuildHashInfoForEachFile(mainDirFullPath, ".", hashInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: MainSrvRoutine(): Failed at BuildHashInfoForEachFile(). \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: HashInfo's were built for all files on the SyncDir server. \n");



        // Obtain listening socket.
        
        status = SrvReturnListeningSocket(&srvSock, srvPort);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error at SrvReturnListeningSocket.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Obtained socket and listen is active. \n");



        //
        // LOOP SYNCDIR SERVER-CLIENT CONNECTIONS & UPDATES:
        //


        // Accept connection & Receive updates.
        // - Connection from the SyncDir client is established here.
        // - Once connected, event updates can be received from the SyncDir client.
        // - Another SyncDir client connection can be accepted only if the current connection is closed.

        while (1)
        {
            printf("[SyncDir] Info: Waiting for SyncDir client to connect ...\n");
            opCount = 1;

            sockConnID = accept(srvSock, (struct sockaddr*) &cltAddr, &lenCltAddr);
            if (sockConnID < 0)
            {
                perror("[SyncDir] Error: MainSrvRoutine(): accept() failed for client. Continue accepting connections.\n");
                continue;
            }
            printf("[SyncDir] Info: SyncDir client connected successfully!\n");



            while (1)
            {
                printf("[SyncDir] Info: Waiting for file updates ... \n\n");
                printf("[#%lu] ----------------------------------------\n", opCount);
                opCount ++;

                status = RecvAndExecuteOperationFromClient(mainDirFullPath, hashInfoHMap, sockConnID);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: MainSrvRoutine(): Failed at RecvAndExecuteOperationFromClient(). \n");
                    status = STATUS_FAIL;
                    break;                  
                    // Maybe SyncDir client closed the connection. So jump to a new accept().
                }        
                printf("[SyncDir] Info: Server updated. Operation received from SyncDir client and executed. \n");
            }
        }




        // If here, everything worked well.
        status = SUCCESS_KEEP_WARNING(STATUS_SUCCESS);


    }//--> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: MainSrvRoutine(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: MainSrvRoutine(): Unkown exception.\n");
        status = STATUS_FAIL;
    }



    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // close connection.
        if (0 < sockConnID)
        {
            close(sockConnID);
        }
        sockConnID = 0;

        // close socket.
        if (0 < srvSock)
        {
            close(srvSock);
        }
        srvSock = 0;
    }
    else
    {
        if (0 < sockConnID)
        {
            close(sockConnID);
        }
        sockConnID = 0;

        if (0 < srvSock)
        {
            close(srvSock);
        }
        srvSock = 0;
    }

    return status;
} // MainSrvRoutine()



