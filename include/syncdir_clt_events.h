
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_EVENTS_H_
#define _SYNCDIR_CLT_EVENTS_H_
/*++
Header for the source files related to file events management: APIs for treating and logging file operations, needed before SyncDir server update.
--*/



#include "syncdir_clt_def_types.h"
#include "syncdir_clt_file_info_proc.h"
#include "syncdir_clt_watch_tree.h"

#include <string.h>
#include <unordered_map>
#include <algorithm>

extern "C"
{
    #include "syncdir_clt_watch_manager.h"
}



extern QWORD gWatchesArrayCapacity;
extern QWORD gTimeLimit;  



//
// Interfaces:
//




//
// SendAllFileInfoEventsToServer: From syncdir_clt_data_transfer.h.
//
extern                                                                              // "extern" used for clarity.
SDSTATUS
SendAllFileInfoEventsToServer(
    __in char                                               *MainDirFullPath,
    __inout std::unordered_map<std::string, FILE_INFO>      &FileInfoHMap,
    __in __int32                                            CltSock
    );



//
// InitEventData
//
SDSTATUS
InitEventData(
    __out EVENT_DATA *DataOfEvent
    );
/*++
Description: 
    The routine initializes with neutral values a given EVENT_DATA structure.
Arguments:
    - DataOfEvent: Pointer to the event data to be initialized.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// BuildEventsForAllSubdirFiles
//
SDSTATUS
BuildEventsForAllSubdirFiles(
    __in DWORD          ParentDirWatchIndex,
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __inout unordered_map<std::string, FILE_INFO> &FileInfoHMap    
    );
/*++
Description: 
    The routine recursively builds and emits all the necessary events for the SyncDir synchronization of a directory's content.
    The directory is identified by its directory watch DirWatchIndex. 
Note: 
    The routine does not build an event for the first directory (DirWatchIndex), but only for the inner files and directories.
Arguments:
    - DirWatchIndex: Index in the Watches array representing the DirWatch structure of the first directory.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
    - FileInfoHMap: Reference to the hash map of FILE_INFO structures of all files.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/



//
// CreateStructuresAndEventsForDirMovedToOnly
//
SDSTATUS
CreateStructuresAndEventsForDirMovedToOnly(
    __in char           *DirRelativePath,
    __in char           *DirFullPath,
    __in char           *DirName,
    __in DWORD          EventWatchIndex,
    __inout DIR_WATCH   **Watches, 
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __inout unordered_map<std::string, FILE_INFO> &FileInfoHMap    
    );
/*++
Description: 
    The routine creates all the watches and the events necessary for monitoring and synchronizing a directory and all of its
    content / files / subdirectories (this being usually the case of a MOVED_TO operation for a directory).
    The concerned directory is represented by DirRelativePath, DirFullPath and DirName. EventWatchIndex identifies the watch that recorded
    the event (usually MOVED_TO) of the directory. 
Note: 
    The routine does not create an event for the first directory (DirRelativePath). But only for the inner files and directories.
Arguments:
    - DirRelativePath: Pointer to the relative path of the directory.
    - DirFullPath: Pointer to the full path of the directory.
    - DirName: Pointer to the short name of the directory.
    - EventWatchIndex: Index, in the Watches array, of the directory watch where the event (usually MOVED_TO) occured.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
    - FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// UpdatePathsByCookieForDirMovedFromAndTo
//
SDSTATUS
UpdatePathsByCookieForDirMovedFromAndTo(
    __in DWORD                  MatchingCookie,
    __inout FILE_INFO           *FileInfoOfDirMoved,
    __in char                   *NewDirRelativePath,
    __in char                   *NewDirName,    
    __inout DIR_WATCH_NODE      *NewWatchNodeOfParentDir,
    __inout DIR_WATCH           *Watches,
    __inout DWORD               *NumberOfWatches,
    __in __int32                HInotify,
    __inout std::unordered_map<std::string, FILE_INFO> & FileInfoHMap
    );
/*++
Description: 
    The routine takes care of all the necessary updates related to paths, required after the movement of a directory.
    The directory is identified by FileInfoOfDirMoved, NewDirRelativePath, NewDirName and NewWatchNodeOfParentDir. 
    The updates include changes in the watch tree, watch directories (Watches), and FileInfos (FileInfoHMap). The routine performs
    the updates only for the marked FileInfo's, i.e. the FileInfo's whose "MovementCookie" field is equal to the MatchingCookie value.
Arguments:
    - MatchingCookie: Integer representing the "movement cookie" which was priorly set in the FileInfo's affected by the directory movement.
    - FileInfoOfDirMoved: Pointer to the FileInfo (of FileInfoHMap) associated with the moved directory.
    - NewDirRelativePath: Pointer to the string containing the new relative path of the moved directory.
    - NewDirName: Pointer to the string containing the short new filename of the moved directory.
    - NewWatchNodeOfParentDir: Pointer to the new parent watch node of the moved directory.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
    - FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// ProcessOperationAndAggregate
//
SDSTATUS
ProcessOperationAndAggregate(
    __in OP_TYPE                    OperationType,
    __in_opt struct inotify_event   *Event,
    __in_opt PEVENT_DATA            DataOfEvent,   
    __inout DIR_WATCH               **Watches,
    __inout DWORD                   *NumberOfWatches,
    __in __int32                    HInotify,
    __inout unordered_map<std::string, FILE_INFO> &FileInfoHMap    
    );
/*++
Description: 
    Core of the SyncDir client application. Affects all structures of SyncDir system by itegrating events with one another (aggregation)
    and records all changes in the Syncdir data structures. Concretely, the routine processes a file operation (event), logs the 
    resulted information and performs modifications that reflect all the effects of the event in the SyncDir application (client).
Note: 
    The routine addresses exclusively the client part of SyncDir. It does not handle communications with the SyncDir server.
Note:
    The Event argument and the DataOfEvent argument are mutually excluding each other (1 == 'usage Event' XOR 'usage DataOfEvent). Hence,
    if one of them is not provided (i.e. NULL), the other one must be.
Arguments:
    - OperationType: Enumeration representing the type of the operation.
    - Event: Mandatory if DataOfEvent is not used, NULL otherwise. Pointer to the structure containing the event information.
    - DataOfEvent: Mandatory if Event is not used, NULL otherwise. Pointer to the structure containing the user-defined event data.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
    - FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// ReadEventsAndIdentifyOperations
//
SDSTATUS
ReadEventsAndIdentifyOperations(
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __inout unordered_map <std::string, FILE_INFO> &FileInfoHMap
    );
/*++
Description: 
    The routine reads all the events from the Inotify handle HInotify, identifies the operations associated
    with each event and passes the operations further for processing and logging in the map of FileInfoHMap.
Arguments:
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
    - FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/


//
// WaitForEventsAndProcessChanges
//
extern "C"                                                  // Declare as extern "C", to make it callable by a C compiler. 
SDSTATUS
WaitForEventsAndProcessChanges(
    __in char           *MainDirFullPath,
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __in __int32        CltSock
    );
/*++
Description: 
    The routine waits for events to appear on SyncDir client side directories, then transmits the events for processing 
    and logging, and later decides when to transfer the changes (operations) to the SyncDir server.
    The routine can wait for events an infinite amount of time, or a limited one (if set so, by the user, at SyncDir launch).
    Before sending the changes to server, the routine performs a last check of the HInotify kernel queue, thus aggregating any
    collateral events that may meanwhile appear.
Arguments:
    - MainDirFullPath: Pointer to the string containg the full (absolute) path towards the main directory monitored by SyncDir client application.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
    was achieved, but related issues were encountered (information is logged, thereby).
--*/







#endif //--> #ifndef _SYNCDIR_CLT_EVENTS_H_



