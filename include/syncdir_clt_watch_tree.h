
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_WATCH_TREE_H_
#define _SYNCDIR_CLT_WATCH_TREE_H_
/*++
Header for the source files related to watch node management (DIR_WATCH_NODE's structures): APIs handling the directory watch tree.
--*/



#include "syncdir_clt_def_types.h"

extern "C"
{
    #include "syncdir_clt_watch_manager.h"
}

#include <stack>
#include <queue>
#include <algorithm>


//
// Interfaces:
//


//
// GetChildWatchNodeByDirName
//
SDSTATUS
GetChildWatchNodeByDirName(
    __in PDIR_WATCH_NODE    ParentNode,
    __in char               *DirName,
    __out PDIR_WATCH_NODE   *ChildNode
    );
/*++
Description: 
    The routine searches among the direct sub-nodes of ParentNode for the node with the same name (the DirName field in the
    DIR_WATCH_NODE structure) as the one pointed by DirName. 
    If a node is found, its address is stored at ChildNode, otherwise, NULL is output.
Arguments:
    - ParentNode: The address of the node whose child is to be found.
    - DirName: The address of the string containing the name to be found.
    - ChildNode: The address where the routine outputs the node address, if one is found, or NULL, otherwise.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// DeleteWatchAndNodeOfDir
//
SDSTATUS
DeleteWatchAndNodeOfDir(
    __inout DIR_WATCH_NODE  *DirWatchNode,
    __inout DIR_WATCH       *Watches,
    __inout DWORD           *NumberOfWatches,
    __in __int32            HInotify    
    );
/*++
Description: 
    The routine deletes and destroys the watch node DirWatchNode, its corresponding DIR_WATCH structure (from the Watches array) 
    and removes its directory watch from the HInotify instance.
Arguments:
    - DirWatchNode: Pointer to the watch node whose watch-related structures are destroyed by the routine.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance associated to all the watches in the Watches array.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// FreeAndNullWatchNode
//
extern "C"
SDSTATUS
FreeAndNullWatchNode(
    __inout DIR_WATCH_NODE **WatchNode
    );
/*++
Description: 
    The routine destroys the watch node WatchNode.
Arguments:
    - DirWatchNode: Pointer to the address of the watch node destroyed by the routine.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// DeleteWatchesAndNodesOfSubdirs
//
SDSTATUS
DeleteWatchesAndNodesOfSubdirs(
    __inout DIR_WATCH_NODE  *StartNode,
    __inout DIR_WATCH       *Watches,
    __inout DWORD           *NumberOfWatches,
    __in __int32            HInotify        
    );
/*++
Description: 
    The routine deletes and destroys all the watch-related structures of the subtree starting at the watch node StartNode.
    This includes the DIR_WATCH_NODE structures, the DIR_WATCH structures and removal of the associated Inotify watches for all subdirectories.
Arguments:
    - StartNode: Pointer to the watch node whose subtree is deleted and destroyed by the routine (the node itself included).
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance associated to all the watches in the Watches array.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// InitWatchNode
//
SDSTATUS
InitWatchNode(
    __out DIR_WATCH_NODE *WatchNode
    );
/*++
Description: 
    The routine initializes a watch node with default values.
Arguments:
    - WatchNode: Pointer to the watch node to be initialized.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// AddChildWatchNodeToTree
//
extern "C"                                                                  // Need it in clt_watch_manager.h.
SDSTATUS
AddChildWatchNodeToTree(
    __in BOOL           IsRootNode,
    __in DWORD          ChildWatchIndex,
    __in DWORD          ParentWatchIndex,
    __in char           *DirName,
    __inout DIR_WATCH   *Watches
    );
/*++
Description: 
    The routine adds an already built watch node in the tree of watches.
Arguments:
    - IsRootNode: Indicates if the node is the first node of the tree (root), or not.
    - ChildWatchIndex: Index in the Watches array, corresponding to the watch node that the routine adds to the tree.
    - ParentWatchIndex: Index in the Watches array, corresponding to the parent watch node.
    - DirName: Pointer to the string containing the directory name of the new watch node.    
    - Watches: Pointer to the array of directory watches.
Return value: 
    STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/


//
// CreateWatchNode
//
extern "C"                                                                  // Need it in clt_watch_manager.h.
SDSTATUS
CreateWatchNode(
    __in DWORD          CrtWatchIndex,
    __inout DIR_WATCH   *Watches
    );
/*++
Description: 
    The routine creates the watch node for a given directory watch. The new watch node is associated to the DIR_WATCH structure
    identified by CrtWatchIndex.
Arguments:
    - CrtWatchIndex: Index in the Watches array, corresponding to the new watch node that the routine creates.
    - Watches: Pointer to the array of directory watches.
Return value: 
    STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/


//
// UpdatePathsForSubTreeWatches
//
SDSTATUS
UpdatePathsForSubTreeWatches(
    __inout DIR_WATCH_NODE      *StartNode,
    __inout DIR_WATCH           *Watches
    );
/*++
Description: 
    The routine updates the full paths and relative paths of all directory watches of a watch tree, plus the depths of all
    watch nodes in the tree. 
Arguments:
    - StartNode: Pointer to the watch node where the watch tree starts.
    - Watches: Pointer to the array of directory watches.
Return value: 
    STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/



//
// CheckWatchNodeExistenceForCleanup
//
SDSTATUS
CheckWatchNodeExistenceForCleanup(
    __in char               *DirName,    
    __inout DIR_WATCH_NODE  *WatchNodeOfParentDir,
    __inout DIR_WATCH       *Watches,
    __inout DWORD           *NumberOfWatches,
    __in __int32            HInotify
    );
/*++
Description: 
    The routine checks if there exists a child watch node for WatchNodeOfParentDir with the same name as DirName. If so,
    the routine destroys and deletes the node, along with its all watch-related structures (the DIR_WATCH inside Watches, the Inotify watch
    enabled upon the Inotify instance HInotify).
Arguments:
    - DirName: Pointer to the string containing the directory name of the node to be found.
    - WatchNodeOfParentDir: Pointer to the parent node of the watch node to be found.
    - Watches: Pointer to the array of directory watches.
    - NumberOfWatches: Pointer to the size of the Watches array.
    - HInotify: Descriptor of the Inotify instance containing the Inotify watches of the monitored directories.
Return value: 
    STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/








#endif //--> #ifndef _SYNCDIR_CLT_WATCH_TREE_H_


