
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_WATCH_MANAGER_H_
#define _SYNCDIR_CLT_WATCH_MANAGER_H_
/*++
Header for the source files related to watch structures management.
- APIs managing Inotify watch structures.
- APIs managing directory watch structures (DIR_WATCH's).
--*/



#include "syncdir_clt_def_types.h"



extern QWORD gWatchesArrayCapacity;              // Declare only (extern).



#ifdef __cplusplus                              // Since all routines are compiled with C compiler.
extern "C"                          
{
#endif



//
// Interfaces:
//


// WaitForEventsAndProcessChanges: From syncdir_clt_events.h.
extern                                                                  // "extern" is not mandatory. But for clarity.
SDSTATUS
WaitForEventsAndProcessChanges(
    __in char           *MainDirFullPath,
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __in __int32        CltSock
    );


//
// AddChildWatchNodeToTree: From syncdir_clt_watch_tree.h.
//
extern
SDSTATUS
AddChildWatchNodeToTree(
    __in BOOL           IsRootNode,
    __in DWORD          ChildWatchIndex,
    __in DWORD          ParentWatchIndex,
    __in char           *DirName,
    __inout DIR_WATCH   *Watches
    );


//
// CreateWatchNode: From syncdir_clt_watch_tree.h.
//
extern
SDSTATUS
CreateWatchNode(
    __in DWORD          CrtWatchIndex,
    __inout DIR_WATCH   *Watches
    );


//
// FreeAndNullWatchNode: From syncdir_clt_watch_tree.h.
//
extern
SDSTATUS
FreeAndNullWatchNode(
    __inout DIR_WATCH_NODE **WatchNode
    );



//
// DeleteDirWatchByIndex
//
SDSTATUS
DeleteDirWatchByIndex(
    __in DWORD          DelIndex,
    __inout DIR_WATCH   *Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify
    );
/*++
Description: 
    The routine removes a directory watch (DIR_WATCH structure) from a provided array (Watches) and also removes its
    associated Inotify watch from the provided Inotify instance (HInotify).
Arguments:
    - DelIndex: Index in the Watches array, representing the directory watch to be removed.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// CreateDirWatchForDirectory
//
SDSTATUS
CreateDirWatchForDirectory(
    __in char                   *DirRelativePath,
    __in char                   *DirFullPath,    
    __in_opt PDIR_WATCH_NODE    WatchNodeOfDir,
    __inout DIR_WATCH           **Watches,
    __inout DWORD               *NumberOfWatches,
    __in __int32                HInotify
    );
/*++
Description: 
    The routine creates an Inotify watch and a directory watch (DIR_WATCH) for a given directory path.
    Concretely, the routine uses the Inotify instance HInotify to add a new watch for the directory DirFullPath. The related information 
    is stored in a new directory watch added to the Watches array. 
Note: 
    DirFullPath must be a valid directory, not a directory symbolic link. Otherwise, the watch would be created for the link itself, 
    not for the directory it points to.
Arguments:
    - DirRelativePath: Pointer to the string containing the relative path of the directory.
    - DirFullPath: Pointer to the string containing the full (absolute) path of the directory.
    - WatchNodeOfDir: Optional. Pointer to the corresponding watch node of the directory, if one is created already.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).
Return value:
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).

--*/



//
// GetDirWatchIndexByHandle
//
SDSTATUS
GetDirWatchIndexByHandle(
    __in DWORD      HWatchToFind,
    __in PDIR_WATCH Watches,
    __in DWORD      NumberOfWatches,
    __out DWORD     *WatchIndexFound
    );
/*++
Description: 
    The routine searches for a directory watch (DIR_WATCH) in a given array, that contains a given watch handle (HWatch field value).
    If found, the routine outputs the index of the directory watch at the address pointed by WatchIndexFound.
Arguments:
    - HWatchToFind: Integer representing the watch handle to be found.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Size of the Watches array.
    - WatchIndexFound: Pointer to where the routine outputs the index of the directory watch having the HWatch field equal to HWatchToFind argument.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// InitDirWatchArray
//
SDSTATUS
InitDirWatchArray(
    __inout DIR_WATCH   *Watches,
    __in    DWORD       ArrayCapacity
    );
/*++
Description: 
    The function initializes with neutral (default) values an array of directory watches.
Arguments:
    - Watches: Pointer to the array of directory watches.
    - ArrayCapacity: Length up to which the routine initializes the Watches array (in number of array elements).
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// ResizeDirWatchArray
//
SDSTATUS
ResizeDirWatchArray(
    __inout DIR_WATCH   **Watches,
    __in    DWORD       NumberOfWatches
    );
/*++
Description: 
    The function allocates more space for the array pointed by (*Watches), increasing its current capacity (gWatchesArrayCapacity) 
    by adding SD_INITIAL_NR_OF_WATCHES elements.
Arguments:
    - Watches: Address of the pointer to the array of DIR_WATCH to be resized.
    - NumberOfWatches: The number of DIR_WATCH elements in the (*Watches) array.
Return value: 
    STATUS_SUCCESS on success. STATUS_FAIL otherwise.
--*/



//
// CreateWatchStructuresForAllSubdirectories
//
SDSTATUS
CreateWatchStructuresForAllSubdirectories(
    __in    DWORD       RootDirWatchIndex, 
    __inout DIR_WATCH   **Watches, 
    __inout DWORD       *NumberOfWatches,
    __in    __int32     HInotify    
    );
/*++
Description: 
    The routine recursively creates all the watch structures for a given directory subtree, necessary for integration
    in the SyncDir monitoring process. The watch structures include Inotify watches, directory watches (DIR_WATCH) and watch nodes
    (DIR_WATCH_NODE). The directory subtree is represented by RootDirWatchIndex argument.
Note: 
    Process not including the first directory! The routine does not create data structures for the root directory (RootDirWatchIndex),
    but only for its contents. The routine assumes the root's structures already exist.
Note: 
    RootDirWatchIndex does not denote the system's root ("/"), nor the main directory of SyncDir monitoring.
Arguments:
    - RootDirWatchIndex: Index, in the Watches array, of the WatchDir where the directory subtree starts.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).

--*/


//
// CltMonitorPartition
//
SDSTATUS
CltMonitorPartition(
    __in char       *MainDirPath,
    __in __int32    CltSock
    );
/*++
Description: 
    Main SyncDir client routine, where the monitoring begins. The routine creates the initial SyncDir data structures and starts the
    monitoring of the main directory (MainDirPath). The routine then launches procedures for monitoring the whole directory tree of MainDirPath, 
    processing the events and updating the SyncDir server. This maintains the synchronization of content between client and server partitions.
Arguments:
    - MainDirPath: Pointer to the string containing the path towards the directory tree that SyncDir will monitor. This can be a file system 
        partition.
    - CltSock: Descriptor representing the socket connection with the SyncDir server application.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).

--*/



#ifdef __cplusplus
}                                                                       //--> Closing the extern "C" directive.
#endif









#endif //--> #ifndef _SYNCDIR_CLT_WATCH_MANAGER_H_


