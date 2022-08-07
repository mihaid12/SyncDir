
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_SRV_DATA_TRANSFER_H_
#define _SYNCDIR_SRV_DATA_TRANSFER_H_



#include "syncdir_srv_def_types.h"
#include "syncdir_srv_hash_info_proc.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


//
// Interfaces:
//


//
// SrvReturnListeningSocket
//
extern "C"                                                                  // Need it in syncdir_srv_main.h, so make it callable by C compiler.
SDSTATUS
SrvReturnListeningSocket(
    __out __int32   *SrvSock,
    __in __int32    SrvPort
    );
/*++   
Description: 
    Creates the server socket and executes listening on all server interfaces and port SrvPort. The routine saves the socket
    at the address pointed by the SrvSock argument.
Arguments:
    - SrvSock: Pointer to where the server socket ID will be stored before routine completion. Storage space must be provided by the caller.
    - SrvPort: Port for the server to wait on for incomming requests.
Return value: 
    STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/



//
// RecvPacketOpAndFilePathFromClient
//
SDSTATUS
RecvPacketOpAndFilePathFromClient(
    __out char          *RelativePath,
    __out PACKET_OP     *OpReceived,
    __in DWORD          SockConnID
    );
/*++
Description: 
    The routine receives an operation packet (PACKET_OP) from a SyncDir client and outputs the operation in the OpReceived
    parameter.
Arguments:
    - RelativePath: Pointer to where the relative path of the file will be stored. The caller must provide the storage space.
    - OpReceived: Pointer to where the operation packet will be stored. The caller must provide the storage space.
    - SockConnID: Descriptor representing the socket connection with the SyncDir client application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// RecvFileFromClient
//
SDSTATUS
RecvFileFromClient(
    __in char       *FileFullPath,
    __out DWORD     *FileSize,
    __in DWORD      SockConnID
    );
/*++
Description: 
    The routine completes the reception of a whole file sent by a SyncDir client application. The file content is stored at 
    the location pointed by FileFullPath and the size of the file is output at the FileSize address.
Arguments:
    - FileFullPath: Pointer to the full path where the routine stores the received file.
    - FileSize: Pointer to where the routine outputs the size of the received file. The caller must provide the storage space. 
    - SockConnID: Descriptor representing the socket connection with the SyncDir client application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



//
// RecvAndExecuteOperationFromClient
//
extern "C"                                                                  // Need it in syncdir_srv_main.h, so make it callable by C compiler.
SDSTATUS
RecvAndExecuteOperationFromClient(
    __in char                                           *MainDirFullPath,
    __inout std::unordered_map<std::string, HASH_INFO>  & HashInfoHMap,
    __in DWORD                                          SockConnID
    );
/*++
Description: 
    The routine receives, stores and executes the operation sent by a SyncDir client application, inside the MainDirFullPath 
    directory. Information related to file modifications (such as hash codes, file sizes) are stored in the HashInfoHMap structure.
Arguments:
    - MainDirFullPath: Pointer to the full path of the server main directory, where the file operations are executed (the server 
    application sees this directory as its own "root" path).
    - HashInfoHMap: Reference to the structure containing the file hash information, including file paths, hash codes and file sizes.
    - SockConnID: Descriptor representing the socket connection with the SyncDir client application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/





#endif //--> #ifndef _SYNCDIR_SRV_DATA_TRANSFER_H_

