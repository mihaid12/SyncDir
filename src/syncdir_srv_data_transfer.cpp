
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_srv_data_transfer.h"



//
// SrvReturnListeningSocket
//
SDSTATUS
SrvReturnListeningSocket(
    __out __int32   *SrvSock,
    __in __int32    SrvPort
    )
/*++   
* Description: Creates the server socket and executes listening on all server interfaces and port SrvPort. The routine saves the socket
* at the address pointed by the SrvSock argument.
*
* - SrvSock: Pointer to where the server socket ID will be stored before routine completion. Storage space must be provided by the caller.
* - SrvPort: Port for the server to wait on for incomming requests.
*
* Return value: STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/
{
    struct sockaddr_in      srvAddr;                                                // Server address info.
    BOOL                    bReuseAddr;                                             // Indicates reusage of address/port after server restart.
    SDSTATUS                status;                 
    __int32                 returnValue;


    // PREINIT. No calls.

    bReuseAddr = FALSE;
    status = STATUS_FAIL;
    returnValue = -1;


    // Validate parameters.

    if (NULL == SrvSock)
    {
        printf("[SyncDir] Error: SrvReturnListeningSocket(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if ((SrvPort < 1024) || (65535 < SrvPort))
    {
        printf("[SyncDir] Error: SrvReturnListeningSocket(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }


    __try
    {

        // INIT.

        // Initialize with TRUE for reusage of same address/port after server shutdown + restart.

        bReuseAddr = TRUE;

        // Initialize server address. Receive requests on all network interfaces (INADDR_ANY).

        memset(&srvAddr, 0, sizeof(srvAddr));
        srvAddr.sin_family = AF_INET;
        srvAddr.sin_port = htons(SrvPort);
        srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);




        //
        // Main processing.
        //

        // Socket creation.

        (*SrvSock) = socket(AF_INET, SOCK_STREAM, 0);
        if ((*SrvSock) < 0)
        {
            perror("[SyncDir] Error: SrvReturnListeningSocket(): Error at socket creation.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Socket created successfully!\n");


        // Set SO_REUSEADDR to enable port reusage after socket close.
        
        returnValue = setsockopt((*SrvSock), SOL_SOCKET, SO_REUSEADDR, &bReuseAddr, sizeof(bReuseAddr));
        if (returnValue < 0)
        {
            perror("[SyncDir] Error SrvReturnListeningSocket(): Error at setsockopt.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Bind socket to srvAddr.

        returnValue = bind((*SrvSock), (struct sockaddr*) &srvAddr, sizeof(srvAddr));
        if (returnValue < 0)
        {
            perror("[SyncDir] Error: SrvReturnListeningSocket(): Error at binding socket.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Socket binded successfully to server address!\n");


        // Listen for incomming requests.

        returnValue  = listen((*SrvSock), SD_MAX_CONNECTIONS);
        if (returnValue < 0)
        {
            perror("[SyncDir] Error: SrvReturnListeningSocket(): Error at listening on socket.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Listening started successfully!\n");



        // If here, everything worked fine.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: SrvReturnListeningSocket(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SrvReturnListeningSocket(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // Cleanup. UNINIT.
    if (SUCCESS(status))        
    {
        // Nothing to clean here.
    }
    else                    // fail
    {
        // Close socket.
        if (0 < (*SrvSock))
        {
            close((*SrvSock));
        }
    }

    return status;
} // SrvReturnListeningSocket




//
// RecvPacketOpAndFilePathFromClient
//
SDSTATUS
RecvPacketOpAndFilePathFromClient(
    __out char          *RelativePath,
    __out PACKET_OP     *OpReceived,
    __in DWORD          SockConnID
    )
/*++
Description: The routine receives an operation packet (PACKET_OP) from a SyncDir client and outputs the operation in the OpReceived
parameter.

- RelativePath: Pointer to where the relative path of the file will be stored. The caller must provide the storage space.
- OpReceived: Pointer to where the operation packet will be stored. The caller must provide the storage space.
- SockConnID: Descriptor representing the socket connection with the SyncDir client application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    __int32         recvBytes;

    // PREINIT.

    status = STATUS_FAIL;
    recvBytes = -1;

    // Parameter validation.

    if (NULL == RelativePath)
    {
        printf("[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == OpReceived)
    {
        printf("[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Receive operation.

        recvBytes = recv(SockConnID, OpReceived, sizeof(PACKET_OP), 0);
        if (sizeof(PACKET_OP) != recvBytes)
        {
            if (recvBytes < 0)
            {
                perror("[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Error at receiving from server (1st). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Warning: RecvPacketOpAndFilePathFromClient(): Numbers of received/to-receive bytes do not match.\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();            
        }            
        fprintf(g_SD_STDLOG, "[SyncDir] Info: Operation (PACKET_OP) received successfully from client.\n");

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Received operation info from client: \n");
        fprintf(g_SD_STDLOG, "- Operation type: [%d], File type: [%d] \n", OpReceived->OperationType, OpReceived->FileType);
        fprintf(g_SD_STDLOG, "- Relative path length: [%d] \n", OpReceived->RelativePathLength);
        fprintf(g_SD_STDLOG, "- Real relative path length: [%d] \n", OpReceived->RealRelativePathLength);
        fprintf(g_SD_STDLOG, "- Old relative path length: [%d] \n", OpReceived->OldRelativePathLength);


        // Receive file relative path.

        recvBytes = recv(SockConnID, RelativePath, OpReceived->RelativePathLength + 1, 0);
        if (OpReceived->RelativePathLength + 1 != recvBytes)
        {
            if (recvBytes < 0)
            {
                perror("[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Error at receiving from server (2nd). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Warning: RecvPacketOpAndFilePathFromClient(): Numbers of received/to-receive bytes do not match.\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();            
        }    
        fprintf(g_SD_STDLOG, "[SyncDir] Info: File relative path received successfully from client.\n");


        // If here, everything is ok.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: RecvPacketOpAndFilePathFromClient(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // RecvPacketOpAndFilePathFromClient()






//
// RecvFileFromClient
//
SDSTATUS
RecvFileFromClient(
    __in char       *FileFullPath,
    __out DWORD     *FileSize,
    __in DWORD      SockConnID
    )
/*++
Description: The routine receives a whole file from a SyncDir client application. The file content is stored at 
the location pointed by FileFullPath and the size of the file is output at the FileSize address.

- FileFullPath: Pointer to the full path where the routine stores the received file.
- FileSize: Pointer to where the routine outputs the size of the received file. The caller must provide the storage space. 
- SockConnID: Descriptor representing the socket connection with the SyncDir client application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS    status;
    FILE        *fileStream;
    __int32     recvBytes;
    DWORD       totalRecvBytes;
    DWORD       writtenBytes;
    PACKET_FILE packet;

    // PREINIT.

    status = STATUS_FAIL;
    fileStream = NULL;
    recvBytes = -1;
    totalRecvBytes = 0;
    writtenBytes = 0;
    packet.ChunkSize = 0;
    packet.FileChunk[0] = 0;

    // Parameter validation.

    if (NULL == FileFullPath || 0 == FileFullPath[0])
    {
        printf("[SyncDir] Error: RecvFileFromClient(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == FileSize)
    {
        printf("[SyncDir] Error: RecvFileFromClient(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    

    __try
    {
        // INIT (start).

        // Log.

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Receiving file from client. Writing at full path [%s]. \n", FileFullPath);

        // Initialize file stream.
        // Open file / create.

        fileStream = fopen(FileFullPath, "w+b");
        if (NULL == fileStream)
        {
            perror("[SyncDir] Error: RecvFileFromClient(): Error at file opening / creation. \n");
            status = STATUS_FAIL;
            throw SyncDirException();              
        }
        
        // Init total bytes.
        
        totalRecvBytes = 0;

        // --> INIT (end)



        //
        // Main procecssing:
        //

        // Receive file size.

        recvBytes = recv(SockConnID, FileSize, sizeof(DWORD), 0);
        if (sizeof(DWORD) != recvBytes)
        {
            if (recvBytes < 0)
            {
                perror("[SyncDir] Error: RecvFileFromClient(): Error at receiving from client (file size). Abandoning file reception.\n");
            }
            else
            {
                printf("[SyncDir] Error: RecvFileFromClient(): Numbers of received/to-receive bytes do not match.\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();                  
        }

        // Network to host. Byte order.

        (*FileSize) = ntohl((*FileSize));

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Receiving file of size [%u]. \n", (*FileSize));


        // Receive whole file content.
        // Write to the open file stream.
        // Note: in case of error recv() returns -1, so if number of bytes received > 0, then the loop continues to receive.
        //      In case of zero bytes received, maybe EOF was met or the socket was shutdown. Hence, loop finishes.

        while (1)
        {

            recvBytes = recv(SockConnID, &packet, sizeof(packet), 0);
            if (sizeof(packet) != (DWORD) recvBytes)
            { 
                if (recvBytes < 0)
                {
                    perror("[SyncDir] Error: RecvFileFromClient(): Error at receiving from client (file chunk). Abandoning file receiving.\n");
                }
                else
                {
                    printf("[SyncDir] Error: RecvFileFromClient(): %d/%lu(MAX) bytes received.\n", recvBytes, sizeof(packet));
                }      
                status = STATUS_FAIL;
                throw SyncDirException();  
            }


            // In case #bytes do not match, fwrite returned error or EOF was met ==> print with perror() in this case.
            // (error can be detected also with "if 0 != ferror(fileStream), then print(error)")

            writtenBytes = fwrite(packet.FileChunk, 1, packet.ChunkSize, fileStream);
            if (packet.ChunkSize != writtenBytes)
            {
                perror("[SyncDir] Error: RecvFileFromClient(): Possible error at fwrite(). \n");
                printf("[SyncDir] Error: RecvFileFromClient(): Only %d/%d bytes written to file.\n", writtenBytes, packet.ChunkSize);
                status = STATUS_FAIL;
                throw SyncDirException();
            }


            // Exit condition. If file completely received (EOF was met).

            if (TRUE == packet.IsEOF)
            {
                fprintf(g_SD_STDLOG, "[SyncDir] Info: RecvFileFromClient(): EOF was met. End transfer. \n");

                (*FileSize) = totalRecvBytes + packet.ChunkSize;                            // Save new file size, before exit.
                break;                
            }


            totalRecvBytes = totalRecvBytes + packet.ChunkSize;

        }//--> while (1)



        // Log.

        fprintf(g_SD_STDLOG, "[SyncDir] Info: File received. \n");


        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: RecvFileFromClient(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: RecvFileFromClient(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        if (NULL != fileStream)
        {
            fclose(fileStream);
            fileStream = NULL;
        }
    }
    else
    {
        if (NULL != fileStream)
        {
            fclose(fileStream);
            fileStream = NULL;
        }
    }

    return status;
} // RecvFileFromClient()




//
// RecvAndExecuteOperationFromClient
//
SDSTATUS
RecvAndExecuteOperationFromClient(
    __in char                                               *MainDirFullPath,
    __inout std::unordered_map<std::string, HASH_INFO>      & HashInfoHMap,
    __in DWORD                                              SockConnID
    )
/*++
Description: The routine receives, stores and executes the operation sent by a SyncDir client application, inside the MainDirFullPath 
directory. Information related to file modifications (such as hash codes, file sizes) are stored in the HashInfoHMap structure.

- MainDirFullPath: Pointer to the full path of the server main directory, where the file operations are executed (the server 
application sees this directory as its own "root" path).
- HashInfoHMap: Reference to the structure containing the file hash information, including file paths, hash codes and file sizes.
- SockConnID: Descriptor representing the socket connection with the SyncDir client application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS            status;
    PACKET_OP           opReceived;
    char                fileRelativePath[SD_MAX_PATH_LENGTH];
    char                fileOldRelativePath[SD_MAX_PATH_LENGTH];
    char                fileRealRelativePath[SD_MAX_PATH_LENGTH];
    char                fileFullPath[SD_MAX_PATH_LENGTH];
    char                fileOldFullPath[SD_MAX_PATH_LENGTH];
    char                fileRealFullPath[SD_MAX_PATH_LENGTH];
    char                fileHashCode[SD_HASH_CODE_LENGTH + 1];    
    char                bufferOut[SD_SHORT_MSG_SIZE];
    char                shellCommand[4 * SD_MAX_PATH_LENGTH];
    char                fileToCopyFullPath[SD_MAX_PATH_LENGTH];
    __int32             recvBytes;
    __int32             sentBytes;
    DWORD               fileSize;
    std::string         auxString;
    std::unordered_map<std::string, HASH_INFO>::const_iterator iteratorHI;

    // PREINIT.

    status = STATUS_FAIL;
    fileRelativePath[0] = 0;
    fileOldRelativePath[0] = 0;
    fileRealRelativePath[0] = 0;
    fileFullPath[0] = 0;
    fileOldFullPath[0] = 0;
    fileRealFullPath[0] = 0;
    fileHashCode[0] = 0;
    bufferOut[0] = 0;
    shellCommand[0] = 0;
    fileToCopyFullPath[0] = 0;
    recvBytes = -1;
    sentBytes = -1;
    fileSize = 0;

    // Parameter validation.

    if (NULL == MainDirFullPath || 0 == MainDirFullPath[0])
    {
        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }    


    __try
    {
        // INIT.
        // --



        //
        // Main processing:
        //

        // I. Receive event information from client.
        // II. Process event. Perform SyncDir data updates.
        // III. Execute file operation on the server.



        // I.
        // Receive operation info and file path.

        status = RecvPacketOpAndFilePathFromClient(fileRelativePath, &opReceived, SockConnID);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute RecvPacketOpAndFilePathFromClient(). \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        // Form full file path.

        sprintf(fileFullPath, "%s/%s", MainDirFullPath, fileRelativePath + 2);
        fprintf(g_SD_STDLOG, "[SyncDir] Info: Success. Operation received for path [%s]. Continue to filtering and server update.\n", 
            fileFullPath);



        // II.
        // Filter operation. Process event and updates.

        switch (opReceived.OperationType)
        {


            //
            // opDELETE
            // opMOVEDFROM
            //

            case (opDELETE):            
            case (opMOVEDFROM):            


                // If file is directory.
                // Delete all HashInfo's of files inside the directory.

                if (ftDIRECTORY == opReceived.FileType)
                {
                    sprintf(shellCommand, "rm -r \"%s\" ", fileFullPath);
                    

                    status = UpdateOrDeleteHashInfosForDirPath(fileFullPath, fileRelativePath, NULL, "DELETE", HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Warning: RecvAndExecuteOperationFromClient(): Failed to execute UpdateOrDeleteHashInfosForDirPath() "
                            "for DELETE, for file [%s]. \n", fileFullPath);
                        status = STATUS_WARNING;
                        // Warning, not Fail, since the file maybe didn't exist before the events.
                    }
                }


                // If file is non-directory.
                // Delete the HashInfo structure of the file.

                if (ftDIRECTORY != opReceived.FileType)
                {
                    sprintf(shellCommand, "rm \"%s\" ", fileFullPath);


                    status = DeleteHashInfoOfFile(fileRelativePath, HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Warning: RecvAndExecuteOperationFromClient(): Failed to execute DeleteHashInfoOfFile() for "
                            "file [%s]. \n", fileRelativePath);
                        status = STATUS_WARNING;
                        // Warning, not Fail, since the file maybe didn't exist before the events.
                    }
                }

                break;



            //
            // opMODIFY
            // opFILMOVEDTO
            //

            case (opMODIFY):
            case (opFILMOVEDTO):



                // Receive the file hash code.

                recvBytes = recv(SockConnID, fileHashCode, SD_HASH_CODE_LENGTH + 1, 0);
                if (SD_HASH_CODE_LENGTH + 1 != recvBytes)
                {
                    if (recvBytes < 0)
                    {
                        perror("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at receiving from server (fileHashCode). \n");
                    }
                    else
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at reception of file hash code. Only %d/%d bytes "
                            "received.\n", recvBytes, SD_HASH_CODE_LENGTH + 1);                      
                    }
                    status = STATUS_FAIL;
                    throw SyncDirException();                      
                }



                // Check if there is file with same hash code on the server.
                // If so, no need to receive the file from client (the server performs a local file copy).
                // If not, receive the file from client.

                auxString.assign(fileHashCode);                                 // Transform to string. Equivalent to operator=(const char *).

                iteratorHI = HashInfoHMap.find(auxString);
                if (HashInfoHMap.end() != iteratorHI)
                {


                    fprintf(g_SD_STDLOG, "[SyncDir] Info: File content is on the server. Preparing a local copy ... \n");


                    // Send info message to client.
                    // No need for file transfer.

                    sprintf(bufferOut, "File On Server");

                    sentBytes = send(SockConnID, bufferOut , SD_SHORT_MSG_SIZE, 0);
                    if (SD_SHORT_MSG_SIZE != (DWORD)sentBytes)
                    {
                        if (sentBytes < 0)
                        {
                            perror("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at sending to server (File on Server).\n");
                        }
                        else
                        {
                            printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at sending to server (File On Server). "
                                "Only %d/%d bytes sent.\n", sentBytes, SD_SHORT_MSG_SIZE);
                        }
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }


                    // Form the copy command for later. (+2 to avoid "./")

                    sprintf(fileToCopyFullPath, "%s/%s", MainDirFullPath, iteratorHI->second.FileRelativePath.c_str() + 2);
                    sprintf(shellCommand, "yes | /bin/cp \"%s\" \"%s\" ", fileToCopyFullPath, fileFullPath);


                    // Insert new HashInfo for the new file copy.

                    status = InsertHashInfoOfFile(fileRelativePath, fileHashCode, fileSize, HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute InsertHashInfoOfFile() for "
                            "file [%s]. \n", fileRelativePath);
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    


                }
                //else
                // 
                if (HashInfoHMap.end() == iteratorHI)
                {


                    fprintf(g_SD_STDLOG, "[SyncDir] Info: File not on the server. Preparing to receive content ... \n");


                    // Send info message to client.
                    // Need to receive the file.

                    sprintf(bufferOut, "File Not On Server");

                    sentBytes = send(SockConnID, bufferOut, SD_SHORT_MSG_SIZE, 0);
                    if (SD_SHORT_MSG_SIZE != (DWORD)sentBytes)
                    {
                        if (sentBytes < 0)
                        {
                            perror("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at sending to server (File Not on Server).\n");
                        }
                        else
                        {
                            printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at sending to server (File Not On Server). "
                                "Only %d/%d bytes sent.\n", sentBytes, SD_SHORT_MSG_SIZE);
                        }
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }


                    // Receive the file from client.

                    status = RecvFileFromClient(fileFullPath, &fileSize, SockConnID);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute RecvFileFromClient() for "
                            "file [%s]. \n", fileRelativePath);
                        status = STATUS_WARNING;
                        // Just warning, because maybe the transfer was interrupted (e.g. in case of volatile files).
                    }                       

                    // Insert new HashInfo for the new received file.

                    status = InsertHashInfoOfFile(fileRelativePath, fileHashCode, fileSize, HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute InsertHashInfoOfFile() for "
                            "file [%s]. \n", fileRelativePath);
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    

                
                }//--> if (HashInfoHMap.end() == iteratorHI)

                break;



            //
            // opCREATE
            //

            case (opCREATE):


                // If file is symbolic link ==> receive its real relative path and form its real full path, for link creation.
                // If file is directory ==> form command (mkdir).
                // If file is non-directory ==> form command (touch).

                switch (opReceived.FileType)
                {

                    case (ftSYMLINK):

                        recvBytes = recv(SockConnID, fileRealRelativePath, opReceived.RealRelativePathLength + 1, 0);
                        if ((DWORD) opReceived.RealRelativePathLength + 1 != (DWORD) recvBytes)
                        {
                            if (recvBytes < 0)
                            {
                                perror("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at receiving from server (symlink). \n");
                            }
                            else
                            {
                                printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at reception of symlink path. Only "
                                    "%d/%d bytes received.\n", recvBytes, opReceived.RealRelativePathLength + 1);                      
                            }
                            status = STATUS_FAIL;
                            throw SyncDirException();                      
                        }


                        sprintf(fileRealFullPath, "%s/%s", MainDirFullPath, fileRealRelativePath + 2);   // +2 for skipping "./" characters.
                        sprintf(shellCommand, "rm \"%s\" ; ln -s \"%s\" \"%s\" ", fileFullPath, fileRealFullPath, fileFullPath);                    

                        break;


                    case (ftDIRECTORY):
                        
                        sprintf(shellCommand, "rm -r \"%s\" ; mkdir \"%s\" ", fileFullPath, fileFullPath);
                        
                        break;


                    default:
                        
                        sprintf(shellCommand, "rm \"%s\" ; touch \"%s\" ", fileFullPath, fileFullPath);                 

                } //--> switch (opReceived.FileType)


                break;



            //
            // MOVE
            // opFILMOVE
            //

            case (opMOVE):
            case (opFILMOVE):


                // Receive the old relative path of the file.

                recvBytes = recv(SockConnID, fileOldRelativePath, opReceived.OldRelativePathLength + 1, 0);
                if (opReceived.OldRelativePathLength + 1 != recvBytes)
                {
                    if (recvBytes < 0)
                    {
                        perror("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at receiving from server (symlink). \n");
                    }
                    else
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Error at reception of symlink path. Only %d/%d bytes "
                            "received.\n", recvBytes, opReceived.OldRelativePathLength + 1);                      
                    }
                    status = STATUS_FAIL;
                    throw SyncDirException();                      
                }


                // Form the old full path and the command.

                sprintf(fileOldFullPath, "%s/%s", MainDirFullPath, fileOldRelativePath + 2);         // +2 for skipping "./" characters.
                sprintf(shellCommand, "mv -T \"%s\" \"%s\" ", fileOldFullPath, fileFullPath);


                // Check if the old path is valid. If not, maybe the old path didn't exist before the events. In this case,
                // the operation resolves to a CREATE, not a MOVE. (possibly followed by other operations, e.g. MODIFY.)

                if (FALSE == IsFileValid(fileOldFullPath))
                {
                    if (ftDIRECTORY != opReceived.FileType)
                    {
                        sprintf(shellCommand, "rm \"%s\" ; touch \"%s\" ", fileFullPath, fileFullPath);
                    }
                    else
                    {
                        sprintf(shellCommand, "rm -r \"%s\" ; mkdir \"%s\" ", fileFullPath, fileFullPath);
                    }

                    break;
                }


                // If file is non-directory.
                // Update file path in the file's HashInfo structure.
                // Note: usage of "-T" option is motivated by operations of directory replacements, where if "-T" is not used,
                //      then the directory is moved INSIDE of the destination one, instead of REPLACING it. Also, the "-T" option 
                //      does not affect the move operation for regular files.

                if (ftDIRECTORY != opReceived.FileType)
                {            
                    status = UpdateHashInfoOfNondirFile(fileOldRelativePath, fileRelativePath, HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute UpdateHashInfoOfNondirFile() for "
                            "new file path [%s] and old file path [%s].\n", fileRelativePath, fileOldRelativePath);
                        status = STATUS_FAIL;
                        throw SyncDirException();                    
                    }                          
                }


                // If file is directory.
                // Update file paths in the HashInfo of every file (due to move operation).

                if (ftDIRECTORY == opReceived.FileType)
                {
                    status = UpdateOrDeleteHashInfosForDirPath(fileOldFullPath, fileOldRelativePath, fileRelativePath, "UPDATE", HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute UpdateOrDeleteHashInfosForDirPath() for "
                            "new file path [%s] and old file path [%s].\n", fileRelativePath, fileOldRelativePath);
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                           
                }                


                break;



            default:

                printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Consistency error at the OperationType.\n");
                status = STATUS_FAIL;
                throw SyncDirException();   


        } // --> switch (opReceived.OperationType)



        // III.
        // Execute the operation.

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Prepare to execute operation on the server file system.\n");
        fprintf(g_SD_STDLOG, "- Command: [%s] \n", shellCommand );

        if (0 != strlen(shellCommand))
        {
            status = ExecuteShellCommand(shellCommand, NULL, 0);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Failed to execute ExecuteShellCommand() for command [%s]. \n",
                    shellCommand);
                status = STATUS_FAIL;
                throw SyncDirException();
            }
        }
        else
        {
            fprintf(g_SD_STDLOG, "[SyncDir] Info: No command to execute. Operations already performed. \n");
        }


        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: RecvAndExecuteOperationFromClient(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: RecvAndExecuteOperationFromClient(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // RecvAndExecuteOperationFromClient()




