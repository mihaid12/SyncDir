
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_MAIN_H_
#define _SYNCDIR_CLT_MAIN_H_
/*++
Header for the source file of the SyncDir client launch routine.
--*/



#include "syncdir_clt_def_types.h"



//extern __int32 gCltSock;       // declaration only (extern).

extern QWORD gTimeLimit;        // declaration only (extern).



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
int main(
    int argc, 
    char **argv
    );



//
// MainCltRoutine
//
SDSTATUS 
MainCltRoutine(
    __in int MainArgc,
    __in char **MainArgv
    );
/*++
Description: 
    The routine verifies and validates all command-line arguments of SyncDir launch (MainArgc and MainArgv arguments), then 
    executes 2 routine calls: CltReturnConnectedSocket and CltMonitorPartition. The former creates a socket connection to the SyncDir server
    and the latter launches the client partition monitoring, which also loops the server update procedures.
Arguments:
    - MainArgc: Length of the MainArgv array, i.e. number of command-line arguments provided at SyncDir client startup. 
    - MainArgv: Pointer to the array of command-line arguments (strings) provided at SyncDir client launch.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



//
// CltReturnConnectedSocket
//
extern                          // From syncdir_clt_data_transfer.h ("extern" used just for clarity).
SDSTATUS
CltReturnConnectedSocket(
    __out   __int32 *CltSock, 
    __in    DWORD   SrvPort, 
    __in    char    *SrvIP
    );



//
// CltMonitorPartition
//
extern                          // From syncdir_clt_watch_manager.h ("extern" used just for clarity).
SDSTATUS
CltMonitorPartition(
    __in char       *MainDirPath,
    __in __int32    CltSock
    );




#ifdef __cplusplus
}                       //--> Closing extern "C".
#endif




#endif //--> #ifndef _SYNCDIR_CLT_MAIN_H_

