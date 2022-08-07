
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_other_miscellaneous.h"



//
// GetDirNodeByWatchHandle
//
DIR_WATCH_NODE*
GetDirNodeByWatchHandle(
    __in PDIR_WATCH_NODE StartNode
    )
{
    DIR_WATCH_NODE *dir;
    dir = NULL;

    if (HWatch == StartNode->HWatch)
    {
        dir = StartNode;
    }
    else
    {
        for (i=0; i<StartNode->NumberOfSubdirs; i++)
        {
            dir = GetDirByWatchHandle(& StartNode->Subdirs[i]);
            if (NULL != dir)
            {
                break;
            }
        }
    }

    return dir;
}


//
// GetDirRelativePath
//
void
GetDirRelativePath(
    __in PDIR_WATCH_NODE    DirNode,
    __out char              *DirRelativePath
    )
{
    char tempRelativePath[SD_MAX_PATH_LENGTH];
    tempRelativePath[0] = 0;

    strcpy(DirRelativePath, DirNode->Name)

    while (1)
    {
        DirNode = DirNode->Parent;
        sprintf(tempRelativePath, "%s/%s", DirNode->Name, DirRelativePath);
        strcpy(DirRelativePath, tempRelativePath);

        if (0 != strcmp(DirNode->Name, "."))
        {
            break;
        }
    }
}




//
// ExistsFdToRead
//
SDSTATUS
ExistsFdToRead(
    __in fd_set         *ReadableFdSet, 
    __in PDIR_WATCH     Watches, 
    __in DWORD          NumberOfWatches,
    __out BOOL          *ExistsFdToRead
    )
{
    DWORD i;
    BOOL isFdSet;

    // PREINIT / INIT.

    i = 0;
    isFdSet = FALSE;

    // Parameter validation.

    if (NULL == ReadableFdSet)
    {
        printf("[SyncDir] Error: ExistsFdToRead(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: ExistsFdToRead(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < NumberOfWatches)
    {
        printf("[SyncDir] Error: ExistsFdToRead(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }

    // Main processing:

    for (i=0; i<NumberOfWatches; i++)
    {
        if (FD_ISSET(Watches[i].HWatch, ReadableFdSet))         // This is redundant. No need to check every watch, but only the Inotify 
        {                                                       // instance to which the watches were added.
            isFdSet = TRUE;
            break;
        }
    }

    // Output result.
    (*ExistsFdToRead) = isFdSet;

    return STATUS_SUCCESS;
} // ExistsFdToRead().


