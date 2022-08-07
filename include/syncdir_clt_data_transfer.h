
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_DATA_TRANSFER_H_
#define _SYNCDIR_CLT_DATA_TRANSFER_H_
/*++
Header for the source files related to all information transfer towards the SyncDir server. Routines where SyncDir client updates the server.
--*/



#include "syncdir_clt_def_types.h"
#include <set>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


//extern __int32 gCltSock;     // Just declaration (extern).



//
// Interfaces:
//


//
// CltReturnConnectedSocket
//
extern "C"				// Need it in syncdir_clt_main.h, so make it callable by C compiler.
SDSTATUS
CltReturnConnectedSocket(
    __out   __int32 *CltSock, 
    __in    DWORD   SrvPort, 
    __in    char    *SrvIP
    );
/*++
Description:
    The routine creates a socket and connects to the address identified by SrvIP and SrvPort.
Arguments:
    - CltSock: Pointer where the socket ID will be output before routine termination.
    - SrvPort: Server port to connect to.
    - SrvIP: Server IP address to connect to (in human readable format "x.x.x.x").
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/


//
// SendFileToServer
//
SDSTATUS
SendFileToServer(
    __in DWORD      FileSize,
    __in char       *FileFullPath,
    __in __int32    CltSock
    );
/*++
Description: 
    The routine sends the content of a file to a server address.
Arguments:
    - FileSize: Size of the file to be sent, in bytes.
    - FileFullPath: Pointer to the string containing the full path of the file (absolute path). 
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value:
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/


//
// SendPacketOpAndFilePathToServer
//
SDSTATUS
SendPacketOpAndFilePathToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in __int32    CltSock
    );
/*++
Description: 
    The routine sends to a server address the following: a packet containing information for an operation and the relative
    path of the file concerned by the operation.
Arguments:
    - OpToSend: Pointer to the packet containing the operation information.
    - FileRelativePath: Pointer to the string containing the relative path of the file (relative to SyncDir main directory).
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// SendCreateToServer
//
SDSTATUS
SendCreateToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in char       *FileRealRelativePath,
    __in __int32    CltSock
    );
/*++
Description: 
    The routine sends a file create operation to a server address.
Arguments:
    - OpToSend: Pointer to the packet containing the operation information.
    - FileRelativePath: Pointer to the relative path of the file (relative to SyncDir main directory).
    - FileRealRelativePath: Pointer to the real relative path of the file (i.e. all subpaths resolved, no symbolic links).
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// SendMoveToServer
//
SDSTATUS
SendMoveToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileNewRelativePath,
    __in char       *FileOldRelativePath,
    __in __int32    CltSock
    );
/*++
Description: 
    The routine sends a file move operation to a server address.
Arguments:
    - OpToSend: Pointer to the packet containing the operation information.
    - FileNewRelativePath: Pointer to the new relative path of the file (relative to SyncDir main directory).
    - FileOldRelativePath: Pointer to the old relative path of the file (relative to SyncDir main directory).
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


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
    );
/*++
Description: 
    The routine sends a file modify operation to a server address.
Arguments:
    - OpToSend: Pointer to the packet containing the operation information.
    - FileRelativePath: Pointer to the relative path of the file (relative to SyncDir main directory).
    - FileFullPath: Pointer to the full path of the file.
    - FileSize: Size of the file concerned by the operation.
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



//
// SendDeleteToServer
//
SDSTATUS
SendDeleteToServer(
    __in PACKET_OP  *OpToSend,
    __in char       *FileRelativePath,
    __in __int32    CltSock
    );
/*++
Description: 
    The routine sends a file delete operation to a server address.
Arguments:
    - OpToSend: Pointer to the packet containing the operation information.
    - FileRelativePath: Pointer to the relative path of the file (relative to SyncDir main directory).
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// InitOperationPacket
//
SDSTATUS
InitOperationPacket(
    __out PACKET_OP *Packet
    );
/*++
Description: 
    The routine initializes to neutral values a PACKET_OP structure.
Arguments:    
    - Packet: Pointer to the structure to be initialized. The caller provides the structure space.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/


//
// PreprocessEventFileInfosBeforeSending
//
SDSTATUS
PreprocessEventFileInfosBeforeSending(
    __inout std::unordered_map<std::string, FILE_INFO>      &FileInfoHMap,
    __out std::set<DWORD>                                   &SetOfDepths  
    );
/*++
Description: 
    The routine performs a preprocessing of the file information related to events, that is found in the FileInfoHMap
    structure. The routine outputs a set containing the depth of each FileInfo's associated file (depth in the directory tree).
Arguments:
    - FileInfoHMap: Reference to the hash map containing the file information related to events (i.e. FILE_INFO structures).
    - SetOfDepths: Reference to the set of depths filled by the routine. Caller provided.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/


//
// SendAllFileInfoEventsToServer
//
SDSTATUS
SendAllFileInfoEventsToServer(
    __in char                                               *MainDirFullPath,
    __inout std::unordered_map<std::string, FILE_INFO>      &FileInfoHMap,
    __in __int32                                            CltSock
    );
/*++
Description: 
    The routine sends all the events recorded in the FILE_INFO structures to a server address.
Arguments:
    - MainDirFullPath: Pointer to the string containg the full (absolute) path towards the main directory monitored by SyncDir client application.
    - FileInfoHMap: Reference to the hash map containing the file information related to system events.
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/




#endif //--> #ifndef _SYNCDIR_CLT_DATA_TRANSFER_H_

