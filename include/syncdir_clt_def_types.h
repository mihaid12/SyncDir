
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIR_CLT_DEF_TYPES_H_
#define _SYNCDIR_CLT_DEF_TYPES_H_
/*++
Header containing the necessary structure types and definitions for the SyncDir client APIs.
--*/



#include "syncdir_essential_def_types.h"

#ifdef __cplusplus                                                  // For C usage as well (for C++ only the extern "C" is needed).
extern "C" 
{
    #include "syncdir_utile.h"
}
#else
    #include "syncdir_utile.h"
#endif

//#include <linux/inotify.h>
#include <sys/inotify.h>

#ifdef __cplusplus
    #include <vector>
#endif 



#ifdef __cplusplus
    extern "C" FILE *g_SD_STDLOG;                                   // C compiler does not recognize this, so use an ifdef.
#else
    extern FILE *g_SD_STDLOG;
#endif



#define SD_MIN_TIME_BEFORE_SYNC     0
#define SD_TIME_TRESHOLD_AT_SYNC    5
#define SD_INITIAL_NR_OF_WATCHES    50

#define SD_EVENT_SIZE (sizeof(struct inotify_event))
#define SD_EVENT_BUFFER_SIZE (1024 * (SD_EVENT_SIZE + NAME_MAX + 1))        // see "man inotify".
#define SD_OPERATIONS_TO_WATCH (IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY)



typedef struct _DIR_WATCH_NODE DIR_WATCH_NODE, *PDIR_WATCH_NODE;



// *********************** C++ only (start) ***********************
#ifdef __cplusplus

//
// DIR_WATCH_NODE - Directory watch node. Stores information related to the monitoring of a directory and the tree structure itself.
//
/*++
As a tree structure, it offers fast (O(1)) path handling or modification (in case of MOVE/CREATE operations).
Though not as fast and straightforward to iterate as a DIR_WATCH array structure, it offers the advantage of 
constant time path modifications (for movements/renamings).
--*/
typedef struct _DIR_WATCH_NODE
{
    DWORD                           DirWatchIndex;              // Index of its associated DirWatch structure (in an array of DIR_WATCH).
    _DIR_WATCH_NODE                 *Parent;                    // One parent (node of parent directory).
    DWORD                           Depth;                      // Depth in the watch tree.
    std::string                     DirName;                    // Directory short name (filename).
    std::vector<_DIR_WATCH_NODE*>   Subdirs;                    // Array of children (PDIR_WATCH_NODE's).
} DIR_WATCH_NODE, *PDIR_WATCH_NODE;

#endif //--> #ifdef __cplusplus
// *********************** C++ only (end) ***********************



//
// DIR_WATCH - Directory watch structure. Stores information related to the Inotify watch of a monitored directory.
//
/*++
Usage as an array:
- Offers fast access to monitored directory information.
- Provides faster iteration than a tree-like structure (both time and space): e.g. no need of additional structure (queue, array, etc.) 
for memorizing (push-ing/pop-ing) tree nodes when iterating sequentially the monitored directories.
--*/
typedef struct _DIR_WATCH
{
    __int32         HWatch;                                 // Handle (descriptor) of an Inotify watch item.
    char            DirRelativePath[SD_MAX_PATH_LENGTH];    // Relative path of the watched directory (relative to SyncDir main directory).
    char            DirFullPath[SD_MAX_PATH_LENGTH];        // Full path (absolute) of the watched directory.
    DIR_WATCH_NODE  *TreeNode;                              // Associated watch node, in the directory watch tree.
} DIR_WATCH, *PDIR_WATCH;



//
// FILE_INFO - File information structure. Aggregates essential information regarding a series of file events.
//
/*++
A FileInfo structure cumulates data of past and present events, i.e. short "history" of a file.
One FileInfo structure stores event data for one file. Past events can be overridden by new ones, if the former do not count anymore.
--*/
typedef struct _FILE_INFO
{
    FILE_TYPE       FileType;
    BOOL            FileExistedBeforeEvents;                            // Helps avoiding redundant operations when processing events.
    DIR_WATCH_NODE  *WatchNodeOfParent;                                 // The watch node of the containing directory.

    char    FileName[255+1];                                            // Short name of the file.
    char    RelativePath[SD_MAX_PATH_LENGTH];                           // File path relative to the main directory.
    char    RealRelativePath[SD_MAX_PATH_LENGTH];                       // Only for sym links: path with all sub-paths resolved.
    char    MD5Hash[32+1];                                              // 128-bit hash + '\0'.
    DWORD   Inode;
    DWORD   FileSize;                                                   // Size of the file, in bytes.
    //BOOL    IsHardLink;                                               // Set upon inode comparison with all file inodes.

    BOOL    WasCreated;                                                 // File was created.
    BOOL    WasDeleted;                                                 // File was deleted.
    BOOL    WasModified;                                                // File content was modified.

    BOOL    WasMovedFromOnly;                                           // File was moved to the outside of the main directory.
    BOOL    WasMovedToOnly; // Deprecated. (Value not of importance, due to latest optimizations.)
    BOOL    WasMovedFromAndTo;                                          // File was moved (inside before, inside after).
    DWORD   MovementCookie;                                             // Flag that matches a MOVED_FROM with a MOVED_TO operation.
    
    DIR_WATCH_NODE  *OldWatchNodeOfParent;                              // The old WatchNodeOfParent field, if file moved.
    char            OldFileName[255+1];                                 // The old FileName field, if file moved.
    char            OldRelativePath[SD_MAX_PATH_LENGTH];                // The old RelativePath field, if file moved.

} FILE_INFO, *PFILE_INFO;



//
// EVENT_DATA - Structure that contains data of one occured file event. 
//
/*++
Note: If required, part of an EventData structure can be filled with data from an Inotify system event (struct inotify_event).
--*/
typedef struct _EVENT_DATA
{
    OP_TYPE OperationType;                                              // Type of the operation.
    char    RelativePath[SD_MAX_PATH_LENGTH];                           // Relative path of the file.
    char    FullPath[SD_MAX_PATH_LENGTH];                               // Full Path of the file.
    char    FileName[255+1];                                            // Short name of the file.
    DWORD   WatchIndex;                                                 // Index, in the WatchDir array, of the containing directory.
    DWORD   Cookie;                                                     // Flag that matches a MOVED_FROM to a MOVED_TO operation.
    BOOL    IsDirectory;                                                // Indicates if file is directory or non-directory.
    BOOL    FileExistedBeforeEvents;                                    // Indicates if file existed before the FileInfo structure creation.

} EVENT_DATA, *PEVENT_DATA;









#endif // _SYNCIDR_CLT_DEF_TYPES_H_

