
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_SRV_MAIN_H_
#define _SYNCDIR_SRV_MAIN_H_
/*++
Header for the source file of the SyncDir server launch routine.
--*/



#include "syncdir_srv_def_types.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>



#ifdef __cplusplus                  
extern "C"                          
{
#endif


//
// Interfaces:
//


//
// main
//
int main(int argc, char **argv);


//
// MainSrvRoutine
//
SDSTATUS MainSrvRoutine(
    __in int MainArgc, 
    __in char **MainArgv
    );
/*++
Description: 
    The routine verifies and validates all command-line arguments of SyncDir launch (MainArgc and MainArgv arguments), then 
    builds all the HashInfo structures for all the existent files (on the server partition) and finally starts accepting a SyncDir client
    connection to receive file updates.
Arguments:
    - MainArgc: Length of the MainArgv array, i.e. number of command-line arguments provided at SyncDir server startup. 
    - MainArgv: Pointer to the array of command-line arguments (strings) provided at SyncDir server launch.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



//
// SrvReturnListeningSocket
//
extern                                                              // From syncdir_srv_data_transfer.h.
SDSTATUS
SrvReturnListeningSocket(
    __out __int32   *SrvSock,
    __in __int32    SrvPort
    );


//
// RecvAndExecuteOperationFromClient
//
extern                                                              // From syncdir_srv_data_transfer.h.
SDSTATUS
RecvAndExecuteOperationFromClient(
    __in char                                           *MainDirFullPath,
    __inout std::unordered_map<std::string, HASH_INFO>  & HashInfoHMap,
    __in DWORD                                          SockConnID
    );


//
// BuildHashInfoForEachFile
//
extern                                                              // From syncdir_srv_hash_info_proc.h.
SDSTATUS
BuildHashInfoForEachFile(
    __in const char * DirFullPath,
    __in const char * DirRelativePath,
    __out unordered_map<std::string, HASH_INFO> & HashInfoHMap    
    );



#ifdef __cplusplus
}                       //--> Closing extern "C".
#endif




#endif //--> #ifndef _SYNCDIR_SRV_MAIN_H_

