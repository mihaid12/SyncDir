
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_OTHER_MISCELLANEOUS_H_
#define _SYNCDIR_OTHER_MISCELLANEOUS_H_
/*++
Header of the source file providing miscellaneous routines, not used in SyncDir programs.
--*/



#include "syncdir_essential_def_types.h"
#include "syncdir_clt_def_types.h"
#include "syncdir_srv_def_types.h"



#ifdef __cplusplus                  // Because it contains functions written in C. So let C++ compiler know how to call these.
extern "C"                          // Need "ifdef __cplusplus", since C compiler does not recognize extern "C".
{
#endif


//
// Interfaces:
//


//
// GetDirNodeByWatchHandle
//
DIR_WATCH_NODE*
GetDirNodeByWatchHandle(
    __in PDIR_WATCH_NODE StartNode
    );


//
// GetDirRelativePath
//
void
GetDirRelativePath(
    __in PDIR_WATCH_NODE    DirNode,
    __out char              *DirRelativePath
    );


//
// ExistsFdToRead
//
SDSTATUS
ExistsFdToRead(
    __in fd_set         *ReadableFdSet, 
    __in PDIR_WATCH     Watches, 
    __in DWORD          NumberOfWatches,
    __out BOOL          *ExistsFdToRead
    );



#ifdef __cplusplus                          //--> closing the extern "C" directive.
}
#endif





#endif //--> #ifndef _SYNCDIR_OTHER_MISCELLANEOUS_H_

