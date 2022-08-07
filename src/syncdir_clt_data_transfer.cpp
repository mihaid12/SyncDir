
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_clt_data_transfer.h"




//
// CltReturnConnectedSocket
//
SDSTATUS
CltReturnConnectedSocket(
    __out   __int32 *CltSock, 
    __in    DWORD   SrvPort, 
    __in    char    *SrvIP
    )
/*++
Description: The routine creates a socket and connects to the address identified by SrvIP and SrvPort.

- CltSock: Pointer where the socket ID will be output before routine termination.
- SrvPort: Server port to connect to.
- SrvIP: Server IP address to connect to (in human readable format "x.x.x.x").

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS            status;    
    struct sockaddr_in  srvAddr;
    BOOL                bReuseAddr;
    __int32             returnValue;

    // PREINIT.

    status = STATUS_FAIL;
    bReuseAddr = TRUE;
    returnValue = -1;

    // Parameter validation.

    if (NULL == CltSock)
    {
        printf("[SyncDir] Error: CltReturnConnectedSocket(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == SrvIP)
    {
        printf("[SyncDir] Error: CltReturnConnectedSocket(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }    
    if ((SrvPort < 1024) || (65535 < SrvPort))
    {
        printf("[SyncDir] Error: CltReturnConnectedSocket(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }        


    __try
    {
        // INIT (start).

        // Indicate reusage of same address/port after client shutdown + restart.
        bReuseAddr = TRUE;

        // Initialize server address to connect to.
        memset(&srvAddr, 0, sizeof(srvAddr));
        srvAddr.sin_family = AF_INET;
        srvAddr.sin_port = htons(SrvPort);
        returnValue = inet_aton(SrvIP, &srvAddr.sin_addr);            // Convert address from ASCII to host format.
        if (0 == returnValue)
        {
            printf("[SyncDir] Error: CltReturnConnectedSocket(): IP address not in valid format.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }    

        // --> INIT (end).



        //
        // Main processing:
        //


        // Socket creation.
        (*CltSock) = socket(AF_INET, SOCK_STREAM, 0);
        if ((*CltSock) < 0)
        {
            perror("[SyncDir] Error: CltReturnConnectedSocket(): Error at socket creation.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Client socket created successfully!\n");

        // Set SO_REUSEADDR to enable address reusage after socket close.
        returnValue = setsockopt((*CltSock), SOL_SOCKET, SO_REUSEADDR, &bReuseAddr, sizeof(bReuseAddr));
        if (returnValue < 0)
        {
            perror("[SyncDir] Error: CltReturnConnectedSocket(): Error at setting socket options.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        // Connect to server.
        returnValue = connect((*CltSock), (struct sockaddr*) &srvAddr, sizeof(srvAddr));
        if (returnValue < 0)
        {
            perror("[SyncDir] Error: CltReturnConnectedSocket(): Error at socket connect.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        printf("[SyncDir] Info: Client connected successfully to the server!\n");



        // If here, everything worked fine.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: CltReturnConnectedSocket(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: CltReturnConnectedSocket(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))        // success 
    {
        // Nothing to clean here 
    }
    else                        // fail
    {
        // Close socket.
        if (0 < (*CltSock))
        {
            close((*CltSock));
            CltSock = NULL;
        }
        (*CltSock) = -1;
    }

    return status;
} // CltReturnConnectedSocket()




//
// SendFileToServer
//
SDSTATUS
SendFileToServer(
    __in DWORD      FileSize,
    __in char       *FileFullPath,
    __in __int32    CltSock
    )
/*++
Description: The routine sends the content of a file to a server address.

- FileSize: Size of the file, in bytes.
- FileFullPath: Pointer to the string containing the full path of the file (absolute path). 
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS        status;
    FILE            *fileStream;
    __int32         sentBytes;
    __int32         readBytes;
    DWORD           totalSentBytes;
    PACKET_FILE     packet;

    // PREINIT.

    status = STATUS_FAIL;
    fileStream = NULL;
    sentBytes = -1;
    readBytes = -1;
    totalSentBytes = 0;
    packet.ChunkSize = 0;
    packet.FileChunk[0] = 0;
    packet.IsEOF = TRUE;

    // Parameter validation.

    if (NULL == FileFullPath || 0 == FileFullPath[0])
    {
        printf("[SyncDir] Error: SendFileToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Sending file of size [%d B] to server. \n", FileSize);


        // Send file size.

        FileSize = htonl(FileSize);                                                     // Machine (local) byte order to network order.
        
        sentBytes = send(CltSock, &FileSize, sizeof(FileSize), 0);
        if (sizeof(FileSize) != sentBytes)
        {
            if (sentBytes < 0)
            {
                perror("[SyncDir] Error: SendFileToServer(): Error at sending to server (file size). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Error: SendFileToServer(): Numbers of sent/to-send bytes do not match (file size).\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Open file.

        fileStream = fopen(FileFullPath, "rb");                                             // Binary file reading.
        if (NULL == fileStream)
        {
            perror("[SyncDir] Error: SendFileToServer(): Error at file opening.\n");
            status = STATUS_WARNING;
            // File may not exist anymore, or has been moved/renamed by the user.

            // Fault tolerance.
            // Send 0 to server (i.e. "end of file receiving").

            packet.IsEOF = TRUE;
            packet.ChunkSize = 0;
            packet.FileChunk[0] = 0;

            sentBytes = send(CltSock, &packet, sizeof(packet), 0);                            // Send packet to server.
            if (sizeof(packet) != sentBytes)
            {
                perror("[SyncDir] Error: SendFileToServer(): Error at sending to server (0, at file open). Abandoning ...\n");
                status = STATUS_FAIL;
                throw SyncDirException();
            }

            // End fault tolerance with warning.
            status = STATUS_WARNING;
            throw SyncDirException();
        }

        totalSentBytes = 0;


        // --> INIT (end).



        //
        // Main processing:
        //



        // Read file chunks.
        // Send file chunks to server.
        // Note: "fread() does not distinguish between end-of-file and error".

        while (1) 
        {
            readBytes = fread(packet.FileChunk, 1, SD_PACKET_DATA_SIZE, fileStream);             // 0 == EOF or Error. 


            packet.ChunkSize = readBytes;
            totalSentBytes = totalSentBytes + packet.ChunkSize; 


            // Exit condition. If EOF was met, or if all file was sent. 
            // If 0, it means EOF or an error appeared (e.g. file was truncated). Abandon transfer.

            if (0 == packet.ChunkSize || packet.ChunkSize < SD_PACKET_DATA_SIZE || (__int32) FileSize - (__int32) totalSentBytes <= 0)
            {                
                packet.IsEOF = TRUE;
            }
            else
            {
                packet.IsEOF = FALSE;
            }


            sentBytes = send(CltSock, &packet, sizeof(packet), 0);                            // Send file chunk to server.
            if (sizeof(packet) != sentBytes)
            {
                if (sentBytes < 0)
                {
                    perror("[SyncDir] Error: SendFileToServer(): Error at sending to server (file chunk). Abandoning ...\n");   
                }
                else
                {
                    printf("[SyncDir] Error: SendFileToServer(): Numbers of sent/to-send bytes do not match (file chunk).\n");
                }
                status = STATUS_FAIL;
                throw SyncDirException();                   
            }


            if (TRUE == packet.IsEOF)
            {
                fprintf(g_SD_STDLOG, "[SyncDir] Info: SendFileToServer(): EOF was met. Ending file transfer. \n");
                break;
            }

        } //--> while (1)



        fprintf(g_SD_STDLOG, "[SyncDir] Info: File sent to server. \n");

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
        cout << "[SyncDir] Error: SendFileToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendFileToServer(): Unkown exception.\n");
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
} // SendFileToServer()


//
// SendPacketOpAndFilePathToServer
//
SDSTATUS
SendPacketOpAndFilePathToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in __int32    CltSock
    )
/*++
Description: The routine sends to a server address the following: a packet containing information for an operation and the relative
path of the file concerned by the operation.

- OpToSend: Pointer to the packet containing the operation information.
- FileRelativePath: Pointer to the string containing the relative path of the file.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    __int32 sentBytes;

    static QWORD opCount = 0;
    
    // PREINIT.

    status = STATUS_FAIL;
    sentBytes = -1;

    // Parameter validation.

    if (NULL == OpToSend)
    {
        printf("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Invalid parameter 1. \n");
        return STATUS_FAIL;        
    }
    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --

        opCount ++;
        fprintf(g_SD_STDLOG, "[#%lu] ----------------------------------------\n", opCount);
        fprintf(g_SD_STDLOG, "[SyncDir] Info: Sending: - Operation type: [%d]. \n", OpToSend->OperationType);


        // 
        // Main processing:
        //



        // Send operation to server.

        sentBytes = send(CltSock, OpToSend, sizeof(PACKET_OP), 0);
        if (sizeof(PACKET_OP) != sentBytes)
        {
            if (sentBytes < 0)
            {
                perror("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Error at sending to server (OpToSend). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Numbers of sent/to-send bytes do not match (OpToSend).\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();            
        }
        fprintf(g_SD_STDLOG, "[SyncDir] Info: Operation (PACKET_OP) was successfully sent to server. \n");



        // Send file relative path to server.

        sentBytes = send(CltSock, FileRelativePath, OpToSend->RelativePathLength + 1, 0);
        if (OpToSend->RelativePathLength + 1 != sentBytes)
        {
            if (sentBytes < 0)
            {
                perror("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Error at sending to server (file path). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Numbers of sent/to-send bytes do not match (file path).\n");
            }
            status = STATUS_FAIL;
            throw SyncDirException();            
        }    
        fprintf(g_SD_STDLOG, "[SyncDir] Info: File relative path was successfully sent to server. \n");



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
        cout << "[SyncDir] Error: SendPacketOpAndFilePathToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendPacketOpAndFilePathToServer(): Unkown exception.\n");
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
} // SendPacketOpAndFilePathToServer()



//
// SendCreateToServer
//
SDSTATUS
SendCreateToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in char       *FileRealRelativePath,
    __in __int32    CltSock
    )
/*++
Description: The routine sends a file create operation to a server address.
Together with the operation information (OpToSend), the routine sends the relative path (FileRelativePath) and the real
relative path (FileRealRelativePath) of the file concerned by the operation.

- OpToSend: Pointer to the packet containing the operation information.
- FileRelativePath: Pointer to the relative path of the file.
- FileRealRelativePath: Pointer to the real relative path of the file (i.e. all subpaths resolved, no symbolic links).
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS status;
    __int32 sentBytes;

    // PREINIT.

    status = STATUS_FAIL;
    sentBytes = -1;

    // Parameter validation.

    if (NULL == OpToSend)
    {
        printf("[SyncDir] Error: SendCreateToServer(): Invalid parameter 1. \n");
        return STATUS_FAIL;        
    }
    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: SendCreateToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }
    if (NULL == FileRealRelativePath || (0 != OpToSend->RealRelativePathLength && 0 == FileRealRelativePath[0]))
    {
        printf("[SyncDir] Error: SendCreateToServer(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Send first 2 information to server:
        // - operation info
        // - file path

        status = SendPacketOpAndFilePathToServer(OpToSend, FileRelativePath, CltSock);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: SendCreateToServer(): Error at 1st or 2nd send to server. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        // Send sym link's real path to server.

        if (ftSYMLINK == OpToSend->FileType)
        {
            sentBytes = send(CltSock, FileRealRelativePath, OpToSend->RealRelativePathLength + 1, 0);
            if (OpToSend->RealRelativePathLength + 1 != sentBytes)
            {
                if (sentBytes < 0)        
                {
                    perror("[SyncDir] Error: SendCreateToServer(): Error at sending to server (real file path). Abandoning ...\n");
                }
                else
                {
                    printf("[SyncDir] Error: SendCreateToServer(): Numbers of sent/to-send bytes do not match (real file path).\n");
                }            
                status = STATUS_FAIL;
                throw SyncDirException(); 
            }
            fprintf(g_SD_STDLOG, "[SyncDir] Info: Symbolic link (+ real path) sent successfully to server. \n");
        }    

        fprintf(g_SD_STDLOG, "[SyncDir] Info: Create operation sent successfully to server. \n");



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
        cout << "[SyncDir] Error: SendCreateToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendCreateToServer(): Unkown exception.\n");
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
} // SendCreateToServer()


//
// SendMoveToServer
//
SDSTATUS
SendMoveToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileNewRelativePath,
    __in char       *FileOldRelativePath,
    __in __int32    CltSock
    )
/*++
Description: The routine sends a file move operation to a server address.
Together with the operation information (OpToSend), the routine sends the new relative path (FileNewRelativePath) and the old 
relative path (FileOldRelativePath) of the file concerned by the operation.

- OpToSend: Pointer to the packet containing the operation information.
- FileNewRelativePath: Pointer to the new relative path of the file.
- FileOldRelativePath: Pointer to the old relative path of the file.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS status;
    __int32 sentBytes;

    // PREINIT.
    status = STATUS_FAIL;
    sentBytes = -1;

    // Parameter validation.

    if (NULL == OpToSend)
    {
        printf("[SyncDir] Error: SendMoveToServer(): Invalid parameter 1. \n");
        return STATUS_FAIL;        
    }
    if (NULL == FileNewRelativePath || 0 == FileNewRelativePath[0])
    {
        printf("[SyncDir] Error: SendMoveToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }
    if (NULL == FileOldRelativePath || 0 == FileOldRelativePath[0])
    {
        printf("[SyncDir] Error: SendMoveToServer(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --

        // Main processing:

        // Send first 2 information to server:
        // - operation info
        // - file path
        
        status = SendPacketOpAndFilePathToServer(OpToSend, FileNewRelativePath, CltSock);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: SendMoveToServer(): Error at 1st or 2nd send to server. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        // Send old path to server (file to be moved on the server).
        
        sentBytes = send(CltSock, FileOldRelativePath, OpToSend->OldRelativePathLength + 1, 0);
        if (OpToSend->OldRelativePathLength + 1 != sentBytes)
        {
            if (sentBytes < 0)        
            {
                perror("[SyncDir] Error: SendMoveToServer(): Error at sending to server (old file path). Abandoning ...\n");
            }        
            else
            {
                printf("[SyncDir] Error: SendMoveToServer(): Numbers of sent/to-send bytes do not match (old file path).\n");
            }                    
            status = STATUS_FAIL;
            throw SyncDirException();
        }


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
        cout << "[SyncDir] Error: SendMoveToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendMoveToServer(): Unkown exception.\n");
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
} // SendMoveToServer()




//
// SendModifyToServer
//
SDSTATUS
SendModifyToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in char       *FileFullPath,
    __in DWORD      FileSize,
    __in __int32    CltSock
    )
/*++
Description: The routine sends a file modify operation to a server address.
Together with the operation information (OpToSend), the routine sends the relative path (FileRelativePath) and the full 
path (FileFullPath) of the file concerned by the operation.

- OpToSend: Pointer to the packet containing the operation information.
- FileRelativePath: Pointer to the relative path of the file (relative to the main directory).
- FileFullPath: Pointer to the full path of the file.
- FileSize: Size of the file concerned by the operation.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS    status;
    __int32     sentBytes;
    __int32     recvReturn;
    char        bufferIn[SD_SHORT_MSG_SIZE];
    char        md5Hash[SD_HASH_CODE_LENGTH + 1];

    // PREINIT.

    status = STATUS_FAIL;
    sentBytes = -1;
    recvReturn = -1;
    bufferIn[0] = 0;
    md5Hash[0] = 0;

    // Parameter validation.

    if (NULL == OpToSend)
    {
        printf("[SyncDir] Error: SendModifyToServer(): Invalid parameter 1. \n");
        return STATUS_FAIL;        
    }
    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: SendModifyToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }
    if (NULL == FileFullPath || 0 == FileFullPath[0])
    {
        printf("[SyncDir] Error: SendModifyToServer(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }



    __try
    {
        // INIT.
        // --


        //
        // Main processing:
        //


        // Get hash of the file.
        
        status = MD5HashOfFile(FileFullPath, md5Hash);
        if (!(SUCCESS(status)))
        {
            perror("[SyncDir] Error: SendModifyToServer(): MD5HashOfFile() failed.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Send first 2 information to server:
        // - operation info
        // - file path
        
        status = SendPacketOpAndFilePathToServer(OpToSend, FileRelativePath, CltSock);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: SendModifyToServer(): Error at 1st or 2nd send to server. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Send hash to server.
        
        sentBytes = send(CltSock, md5Hash, SD_HASH_CODE_LENGTH + 1, 0);
        if (SD_HASH_CODE_LENGTH + 1 != sentBytes)
        {
            if (sentBytes < 0)        
            {
                perror("[SyncDir] Error: SendModifyToServer(): Error at sending to server (MD5 hash). Abandoning ...\n");
            }
            else
            {
                printf("[SyncDir] Error: SendModifyToServer(): Numbers of sent/to-send bytes do not match (MD5 hash).\n");
            }            
            status = STATUS_FAIL;
            throw SyncDirException();  
        }


        // Receive answer from server: File content exists already on server ?

        recvReturn = recv(CltSock, bufferIn, SD_SHORT_MSG_SIZE, 0);
        if (recvReturn < 0)        
        {
            perror("[SyncDir] Error: SendModifyToServer(): Error at receiving from server. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        if (0 == strcmp(bufferIn, "File On Server"))
        {
            // Yes: File contents exist on server.
            // ==> Do nothing. The server will do a local copy.
            fprintf(g_SD_STDLOG, "[SyncDir] Info: Server replied 'file on server'. No action needed. \n");
        }
        if (0 == strcmp(bufferIn, "File Not On Server"))      
        {
            // No: File does not exist on server.
            // ==> Send file.
            fprintf(g_SD_STDLOG, "[SyncDir] Info: Server replied 'file not on server'. Preparing to send file ... \n");
            
            status = SendFileToServer(FileSize, FileFullPath, CltSock);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: SendModifyToServer(): SendFileToServer() failed.\n");
                status = STATUS_WARNING;
                throw SyncDirException();
                // Just warning, for fault tolerance. Sending files can be interrupted if files are volatile (e.g. temporary, backup).
            }
        }


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
        cout << "[SyncDir] Error: SendModifyToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendModifyToServer(): Unkown exception.\n");
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
} // SendModifyToServer()



//
// SendDeleteToServer
//
SDSTATUS
SendDeleteToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in __int32    CltSock
    )
/*++
Description: The routine sends a file delete operation to a server address.
Together with the operation information (OpToSend), the routine sends the new relative path (FileRelativePath) of the file 
concerned by the operation.

- OpToSend: Pointer to the packet containing the operation information.
- FileRelativePath: Pointer to the relative path of the file.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == OpToSend)
    {
        printf("[SyncDir] Error: SendDeleteToServer(): Invalid parameter 1. \n");
        return STATUS_FAIL;        
    }
    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: SendDeleteToServer(): Invalid parameter 2. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Send first 2 information to server:
        // - operation info
        // - file path

        status = SendPacketOpAndFilePathToServer(OpToSend, FileRelativePath, CltSock);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: SendDeleteToServer(): Error at 1st or 2nd send to server. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }           


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
        cout << "[SyncDir] Error: SendDeleteToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: SendDeleteToServer(): Unkown exception.\n");
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
} // SendDeleteToServer()



//
// InitOperationPacket
//
SDSTATUS
InitOperationPacket(
    __out PACKET_OP *Packet
    )
/*++
Description: The routine initializes to neutral values a PACKET_OP structure.

- Packet: Pointer to the structure to be initialized. The caller provides the structure space.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == Packet)
    {
        printf("[SyncDir] Error: InitOperationPacket(): Invalid parameter 1. \n");        
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Set a "preinit" value for each field of the packet.

        Packet->OperationType = opUNKNOWN;
        Packet->FileType = ftUNKNOWN;
        Packet->RelativePathLength = 0;
        Packet->RealRelativePathLength = 0;
        Packet->OldRelativePathLength = 0;


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
        cout << "[SyncDir] Error: InitOperationPacket(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: InitOperationPacket(): Unkown exception.\n");
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
} // InitOperationPacket()



//
// PreprocessEventFileInfosBeforeSending
//
SDSTATUS
PreprocessEventFileInfosBeforeSending(
    __inout std::unordered_map<std::string, FILE_INFO>      &FileInfoHMap,
    __out std::set<DWORD>                                   &SetOfDepths  
    )
/*++
Description: The routine performs a preprocessing of the file information related to events, that is found in the FileInfoHMap
structure. As a result, the routine outputs a set containing the depth of each FileInfo's associated file (depth in the watch tree 
that monitors the system directories).

- FileInfoHMap: Reference to the hash map containing the file information related to events (i.e. FILE_INFO structures).
- SetOfDepths: Reference to the set of depths filled by the routine. Caller provided.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    FILE_INFO *fileInfo;

    // PREINIT.

    status = STATUS_FAIL;
    fileInfo = NULL;

    // Parameter validation.

    if (FileInfoHMap.size() <= 0)
    {
        printf("[SyncDir] Error: PreprocessEventFileInfosBeforeSending(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }
    if (0 != SetOfDepths.size())
    {
        printf("[SyncDir] Error: PreprocessEventFileInfosBeforeSending(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // 
        // Main processing:
        //

        // Collect the depths of all the FileInfo parent nodes in the watch tree.

        for (auto it = FileInfoHMap.begin(); it != FileInfoHMap.end(); it ++)
        {
            fileInfo = &(it->second);

            if (ftDIRECTORY == fileInfo->FileType)
            {
                SetOfDepths.insert(1 + fileInfo->WatchNodeOfParent->Depth);
            }


            // Note: The following action (setting WasDeleted for the "moved from" FileInfos) is commented because it is now performed
            //  directly when updating the server (step 3, in Main Processing, at SendAllFileInfoEventsToServer() ).

            // // Mark "deleted" all FileInfo's of MOVED_FROM operations (i.e. moved outside of SyncDir main directory).
            // if ( TRUE == fileInfo->WasMovedFromOnly && 0 != fileInfo->MovementCookie )      // If moved outside of the main directory.
            // {
            //    fileInfo->WasDeleted = TRUE;
            // }
        }


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
        cout << "[SyncDir] Error: PreprocessEventFileInfosBeforeSending(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: PreprocessEventFileInfosBeforeSending(): Unkown exception.\n");
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
} // PreprocessEventFileInfosBeforeSending()



//
// SendAllFileInfoEventsToServer
//
SDSTATUS
SendAllFileInfoEventsToServer(
    __in char                                               *MainDirFullPath,
    __inout std::unordered_map<std::string, FILE_INFO>      &FileInfoHMap,
    __in __int32                                            CltSock
    )
/*++
Description: The routine sends all the events recorded in the FILE_INFO structures to a server address.

- MainDirFullPath: Pointer to the string containg the full (absolute) path towards the main directory monitored by SyncDir client application.
- FileInfoHMap: Reference to the hash map containing the file information related to system events.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS            status;
    FILE_INFO           *fileInfo;
    FILE_INFO           copyOfFileInfo;
    PACKET_OP           opToSend;
    DWORD               crtFileSize;
    char                crtFileFullPath[SD_MAX_PATH_LENGTH];
    struct stat         crtFileStat;
    std::set<DWORD>     setOfDepths;
    std::set<DWORD>::iterator crtDepth;
    std::unordered_map<std::string, FILE_INFO>::iterator it;

    // PREINIT.

    status = STATUS_FAIL;
    fileInfo = NULL;    
    crtFileSize = 0;
    crtFileFullPath[0] = 0;

    // Parameter validation.

    if (NULL == MainDirFullPath || 0 == MainDirFullPath[0])
    {
        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }


    __try
    {

        // INIT.
        // --

        // Verify if empty.

        if (FileInfoHMap.size() <= 0)
        {
            printf("[SyncDir] Info: SendAllFileInfoEventsToServer(): FileInfo record set is empty. Exiting ...\n");
            return STATUS_SUCCESS;      // Return success.
        }

        // Pre-processing:

        status = PreprocessEventFileInfosBeforeSending(FileInfoHMap, setOfDepths);           // std::set is internally sorted.
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Error at preprocessing the FileInfo's. Abandoning ...\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }  



        //
        // Start main processing:
        //


        // Send all the recorded events inside the FileInfo's to the server address.
        // Begin by sending the FileInfo events of directories (==> the server will firstly apply the operations related to directories):
        // - from lowest directory depth to the highest.
        // After sending all the directory related events, the remaining non-directory file events are sent.


        for (it = FileInfoHMap.begin(), crtDepth = setOfDepths.begin();  FALSE == FileInfoHMap.empty(); )
        {

            fprintf(g_SD_STDLOG, "\n[SyncDir] Info: Iterating event records for transfer (FileInfo's) ... Jumping to next FileInfo. \n");


            if (FileInfoHMap.end() == it)                                           // Restart iterating the remaining FileInfo's.
            {
                if (setOfDepths.end() != crtDepth)                                  // Prioritize the next depth directories.
                {
                    crtDepth ++;
                }
                it = FileInfoHMap.begin();
            }


            fileInfo = &(it->second);                                               // Take the reference to iterated FileInfo.


            if (setOfDepths.end() != crtDepth)
            {
                if (ftDIRECTORY != fileInfo->FileType)                              // If non-dir, ignore for the moment.
                {
                    it ++;
                    continue;
                }
                //else
                if (ftDIRECTORY == fileInfo->FileType)                              // If dir, verify its depth (accept only crtDepth) !
                {
                    if ((*crtDepth) != 1 + fileInfo->WatchNodeOfParent->Depth)      // Only dirs with crtDepth.
                    {
                        it ++;
                        continue;
                    }
                }
            }


            // Save current FileInfo (to be sent to server).
            // Delete current FileInfo from the hash map.

            copyOfFileInfo = it->second;
            it = FileInfoHMap.erase(it);
            fileInfo = & copyOfFileInfo;


            // Initialize operation packet.

            status = InitOperationPacket(&opToSend);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute InitOperationPacket(). Abandoning ...\n");
                status = STATUS_FAIL;
                throw SyncDirException();
            }  


            // Get size and full path of current FileInfo's file.

            sprintf(crtFileFullPath, "%s/%s", MainDirFullPath, fileInfo->RelativePath + 2);                  // +2 to skip "./" 

            if (lstat(crtFileFullPath, &crtFileStat) < 0)
            {
                perror("[SyncDir] Info: SendAllFileInfoEventsToServer(): Could not execute lstat(). \n");
                printf("lstat() info was for file [%s]. \n", crtFileFullPath);
                printf("File may not exist anymore, or the user renamed/moved the file meanwhile. \n");

                crtFileSize = 0;
            }
            else
            {
                crtFileSize = crtFileStat.st_size;
            }


            //
            // Build packet. Add current info to the packet.
            //

            // -> Add file type, file path length.

            opToSend.FileType = fileInfo->FileType;
            opToSend.RelativePathLength = strlen(fileInfo->RelativePath);

            // -> Add symbolic link info (real path length).

            if (ftSYMLINK == fileInfo->FileType)
            {
                opToSend.RealRelativePathLength = strlen(fileInfo->RealRelativePath);
            }
            else
            {
                opToSend.RealRelativePathLength = 0;
            }

            // -> Add move/rename info (old path length).
            
            if (TRUE == fileInfo->WasMovedFromAndTo)
            {
                opToSend.OldRelativePathLength = strlen(fileInfo->OldRelativePath);
            }
            else
            {
                opToSend.OldRelativePathLength = 0;
            }



            //Log.

            if (setOfDepths.end() != crtDepth)
            {
                fprintf(g_SD_STDLOG, "[SyncDir] Info: Sending directory of depth/level: [%u]. \n", (*crtDepth));
            }

            fprintf(g_SD_STDLOG, "[SyncDir] Info: Sending to server operation info: \n");
            fprintf(g_SD_STDLOG, "- Full path: [%s] \n", crtFileFullPath);
            fprintf(g_SD_STDLOG, "- File type: [%d] \n", opToSend.FileType);
            fprintf(g_SD_STDLOG, "- Relative path length: [%d] \n", opToSend.RelativePathLength);
            fprintf(g_SD_STDLOG, "- Relative path: [%s] \n", fileInfo->RelativePath);
            fprintf(g_SD_STDLOG, "- Real relative path length: [%d] \n", opToSend.RealRelativePathLength);
            fprintf(g_SD_STDLOG, "- Real relative path: [%s] \n", fileInfo->RealRelativePath);            
            fprintf(g_SD_STDLOG, "- Old relative path length: [%d] \n", opToSend.OldRelativePathLength);
            fprintf(g_SD_STDLOG, "- Old relative path: [%s] \n", fileInfo->OldRelativePath);
            /**
             * The event check priority:
             * 1. DELETE (WasDeleted)
             * 2. MOVED_FROM (WasMovedFromOnly)
             * 3. MOVED_TO (WasMovedToOnly && ! WasMovedFromAndTo)
             * 4. MOVE (WasMovedFromAndTo) + any modifications ? if/else (WasMovedToOnly || WasModified || WasCreated)
             * 5. MODIFY (WasModified)
             * 6. CREATE (WasCreated)
            **/


            // 1. DELETE
            // Check if file was deleted.
            // Delete makes sense only if the file existed before the events.
            // If file didn't exist, the server does not have it yet. So, no need to inform about the deletion.
            // ** For ftDIRECTORY and ftREGULAR/ftSYMLINK/ftHARDLINK: **


            if (TRUE == fileInfo->WasDeleted)
            {
                if (TRUE == fileInfo->FileExistedBeforeEvents)
                {
                    opToSend.OperationType = opDELETE;

                    status = SendDeleteToServer(&opToSend, fileInfo->RelativePath, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendDeleteToServer() (at DELETE).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }

                continue;
            }


            // 2. MOVED_FROM
            // Check if file was moved outside of the main directory.
            // "Moving from" (deleting) makes sense only if file existed before the events.
            // If file didn't exist, the server does not have it yet. So, no need to inform about the deletion.
            // ** For ftDIRECTORY and ftREGULAR/ftSYMLINK/ftHARDLINK: **


            if (TRUE == fileInfo->WasMovedFromOnly)
            {          
                if (TRUE == fileInfo->FileExistedBeforeEvents)
                {
                    opToSend.OperationType = opMOVEDFROM;
                    
                    status = SendDeleteToServer(&opToSend, fileInfo->RelativePath, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendDeleteToServer() (at MOVED_FROM).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                }

                continue;
            }


            // 3. MOVED_TO
            // Check if file was moved inside, from outside of the main directory.
            // Regular file ==> send file to server.
            // Directory file ==> Do nothing. (Events were already added for treatment.):
                // Corresponding CREATE's and MODIFY's are already in the FileInfo hash map.
                // -> see function CreateStructuresAndEventsForDirMovedToOnly().
                // Therefore, subsequent FileInfo's will produce all the subdirectories and files inside, on the server.
                // (Since FileInfo's were already added for the creation of all the file structure of the directory.)
                // <=> all this is equivalent to a SendDirectoryToServer() function.


            if (TRUE == fileInfo->WasMovedToOnly && FALSE == fileInfo->WasMovedFromAndTo)
            {
                // ** For ftDIRECTORY: **

                if (ftDIRECTORY == fileInfo->FileType)
                {
                    // Do nothing here. (Read above.)
                }


                // ** For ftREGULAR/ftSYMLINK/ftHARDLINK: **

                if (ftDIRECTORY != fileInfo->FileType)
                {
                    opToSend.OperationType = opFILMOVEDTO;
                    
                    status = SendModifyToServer(&opToSend, fileInfo->RelativePath, crtFileFullPath, crtFileSize, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendModifyToServer() (at MOVED_TO).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                }            

                continue;
            }


            // 4. MOVE
            // Check if file was renamed (WasMovedFromAndTo) inside the main directory.
            // Additionally:
            // Check operations that could have modified the file right before.
            // E.g.:
            // - WasMovedTo: mv file_outside main_dir/file_inside
            // - WasModified: echo "" >> file
            // - WasCreated: rm file && touch file; rmdir file_2 && mkdir file_2;
            // 1. If any modifications to the file ==>
            //      ==> Send MOVE + also send MODIFY to the server.
            // 2. If no file modifications ==>
            //      ==> Send only MOVE to the server.
            //
                // In case of previous MOVED_TO or CREATE (for directories), the following applies:
                //      Corresponding CREATE's and MODIFY's are already in the FileInfo hash map.
                //      -> see function CreateStructuresAndEventsForDirMovedToOnly().
                //      Therefore, subsequent FileInfo's will generate all the subdirectories and files inside, on the server.
                //      (Since FileInfo's were already added for the creation of all the file structure of the directory.)


            if (TRUE == fileInfo->WasMovedFromAndTo)
            {
                if (TRUE == fileInfo->WasModified)                          // Check if file was also modified.
                {                                                           // Condition "TRUE == fileInfo->WasMovedToOnly" is obsolete.

                    // ** For ftREGULAR/ftSYMLINK/ftHARDLINK: **
                
                    opToSend.OperationType = opFILMOVE;

                    status = SendMoveToServer(&opToSend, fileInfo->RelativePath, fileInfo->OldRelativePath, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendMoveToServer() (1st, at MOVE).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    

                    opToSend.OperationType = opMODIFY;

                    status = SendModifyToServer(&opToSend, fileInfo->RelativePath, crtFileFullPath, crtFileSize, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendModifyToServer() (at MOVE).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }

                }
                // Else, file was not also modified.
                else                                                    
                {

                    // ** For ftDIRECTORY and ftREGULAR/ftSYMLINK/ftHARDLINK: **

                    opToSend.OperationType = opMOVE;

                    status = SendMoveToServer(&opToSend, fileInfo->RelativePath, fileInfo->OldRelativePath, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendMoveToServer() (2nd, at MOVE).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    

                }//else

                continue;
            }


            // 5. MODIFY
            // Check if file content was modified.
            // (directory files cannot be announced "modified" by Inotify)
            // ** For ftREGULAR/ftSYMLINK/ftHARDLINK (only): **


            if (TRUE == fileInfo->WasModified)
            {
                opToSend.OperationType = opMODIFY;

                status = SendModifyToServer(&opToSend, fileInfo->RelativePath, crtFileFullPath, crtFileSize, CltSock);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendModifyToServer() (at MODIFY).\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }                

                continue;
            }


            // 6. CREATE
            // Check if file was only created.
            // ** For ftDIRECTORY and ftREGULAR/ftSYMLINK/ftHARDLINK: **


            if (TRUE == fileInfo->WasCreated)
            {
                opToSend.OperationType = opCREATE;

                status = SendCreateToServer(&opToSend, fileInfo->RelativePath, fileInfo->RealRelativePath, CltSock);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: SendAllFileInfoEventsToServer(): Failed to execute SendCreateToServer() (at CREATE).\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }                

                continue;
            }

            printf("[SyncDir] Warning: SendAllFileInfoEventsToServer(): Recorded event not recognized. Continuing ...\n");


        } // --> for (auto &fileInfo: FileInfoHMap)


        // Events sent.
        // Clear the event records (file infos).
        FileInfoHMap.clear();


        fprintf(g_SD_STDLOG, "[SyncDir] Info: All event records (FileInfo's) sent to server. \n");



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
        cout << "[SyncDir] Error: SendAllFileInfoEventsToServer(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: Unkown exception.\n");
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
} // SendAllFileInfoEventsToServer()


