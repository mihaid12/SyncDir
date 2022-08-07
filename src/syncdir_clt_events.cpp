
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_clt_events.h"

//using namespace std;                // To be left here. Not in the header. And after all "#includes".




//
// InitEventData
//
SDSTATUS
InitEventData(
    __out EVENT_DATA *DataOfEvent
    )
/*++
Description: The routine initializes to neutral values a given EVENT_DATA structure.

- DataOfEvent: Pointer to the event data to be initialized.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == DataOfEvent)
    {
        printf("[SyncDir] Error: InitEventData(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }


    __try
    {

        // INIT.
        // --


        // Main processing:

        // Initialize all fields of the EVENT_DATA structure.
        DataOfEvent->OperationType = opUNKNOWN;
        DataOfEvent->RelativePath[0] = 0;
        DataOfEvent->FullPath[0] = 0;
        DataOfEvent->FileName[0] = 0;
        DataOfEvent->WatchIndex = 0;
        DataOfEvent->IsDirectory = FALSE;
        DataOfEvent->Cookie = 0;


        // If here, everything is ok.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: InitEventData(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: InitEventData(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.

    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // InitEventData()




//
// BuildEventsForAllSubdirFiles
//
SDSTATUS
BuildEventsForAllSubdirFiles(
    __in DWORD          DirWatchIndex,
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __inout unordered_map<std::string, FILE_INFO> &FileInfoHMap    
    )
/*++
Description: The routine recursively builds and emits all the necessary events for the SyncDir synchronization of a directory's content.
The directory is identified by its directory watch DirWatchIndex. 
All the events built by the routine are passed on to ProcessOperationAndAggregate(), to ensure the correct processing and aggregation 
of the events.

Note: The routine does not build an event for the first directory (DirWatchIndex), but only for the inner files and directories.

- DirWatchIndex: Index in the Watches array representing the DirWatch structure of the first directory.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
- FileInfoHMap: Reference to the hash map of FILE_INFO structures of all files.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    EVENT_DATA      DataOfEvent;
    DIR             *dirStream;
    struct dirent   *file;
    struct stat     fileStat;
    DWORD           indexOfNewDirWatch;

    // PREINIT.

    status = STATUS_FAIL;
    dirStream = NULL;
    file = NULL;
    indexOfNewDirWatch = 0;

    // Parameter validation.

    // Start with parameter 2. The 3rd must be validated before the 1st (see below).
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }
    if ((*NumberOfWatches) <= DirWatchIndex)
    {
        printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Invalid parameter 1.\n");
        return STATUS_FAIL;        
    }


    __try
    {
        // INIT (start).

        // Get directory full path and open it for file iteration.

        dirStream = opendir((*Watches)[DirWatchIndex].DirFullPath);
        if (NULL == dirStream)
        {
            perror("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Could not execute opendir() for the parent directory.\n");
            printf("The error was for path [%s] \n", (*Watches)[DirWatchIndex].DirFullPath);
            status = STATUS_WARNING;
            throw SyncDirException();

            // File may not exist anymore, or the user renamed/moved the file meanwhile.
        }

        // --> INIT (end).


        //
        // Start main processing:
        //


        //
        // LOOP: Recursively iterate all files and subdirectories and build creation events for all (opDIRCREATE, opMODIFY).
        //

        while (1)
        { 

            // Iterate the subdirectories/subfiles of the parent directory (== dirStream). Until the end of directory stream.

            file = readdir(dirStream);
            if (NULL == file)                                               
            {
                break; // End of stream.
            }            

            
            // Get fstatat() with AT_SYMLINK_NOFOLLOW to not resolve symbolic links. So it behaves like lstat().
            
            if (fstatat(dirfd(dirStream), file->d_name, &fileStat, AT_SYMLINK_NOFOLLOW) < 0)
            {
                perror("[SyncDir] Warning: BuildEventsForAllSubdirFiles(): Could not execute fstatat() for the file.\n");
                // Possibly the file was (re)moved meanwhile. (Or altered.)            
                // -> Any file-related modifications will apear on the Inotify watch (already created).

                continue;   // Skip the file.
            }


            // Skip the present directory and the parent directory.

            if (0 == strcmp(".", file->d_name) || 0 == strcmp("..", file->d_name))
            {
                continue;
            }


            // Initialize the data that simulates (builds) a file event of CREATE or MODIFY.

            status = InitEventData(&DataOfEvent);        
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Failed to execute InitEventData(). \n");   
                status = STATUS_FAIL;
                throw SyncDirException();
            }            


            // Set the currently known event data for the current file.
            // FileExistedBeforeEvents is set to FALSE because these are files inside newly created directories, so the paths of
            //      these files didn't exist before the events.

            snprintf(DataOfEvent.RelativePath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[DirWatchIndex].DirRelativePath, file->d_name);
            
            snprintf(DataOfEvent.FullPath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[DirWatchIndex].DirFullPath, file->d_name);

            strcpy(DataOfEvent.FileName, file->d_name);

            DataOfEvent.WatchIndex = DirWatchIndex;
            DataOfEvent.Cookie = 0;                                             // Don't need cookie for CREATE's and MODIFY's.
            DataOfEvent.FileExistedBeforeEvents = FALSE;


            // If file is directory:
            // - emit a DIRCREATE operation (since the directory must be created on the server) -> ProcessOperationAndAggregate().
            // - start recursive call (on the directory) -> BuildEventsForAllSubdirFiles().

            if (S_ISDIR(fileStat.st_mode))
            {

                DataOfEvent.IsDirectory = TRUE;
                DataOfEvent.OperationType = opDIRCREATE;
                
                status = ProcessOperationAndAggregate(opDIRCREATE, NULL, &DataOfEvent, Watches, NumberOfWatches, HInotify, FileInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Failed to execute ProcessOperationAndAggregate() (CREATE). \n");   
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }                     


                // Recursivity.
                // The DirWatch index of the directory created above (with opDIRCREATE) is the last one: NumberOfWatches - 1.

                indexOfNewDirWatch = (*NumberOfWatches) - 1;

                status = BuildEventsForAllSubdirFiles(indexOfNewDirWatch, Watches, NumberOfWatches, HInotify, FileInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Warning: BuildEventsForAllSubdirFiles(): Recursive call failed for a subdirectory. "
                        "Continuing execution... \n");  // Some file may have been removed meanwhile (?).
                    
                    continue;
                }

            }


            // If file is non-directory.
            // - emit a MODIFY operation (for the file should be futurely transfered to the server).

            if (! S_ISDIR(fileStat.st_mode))
            {
                DataOfEvent.IsDirectory = FALSE;
                DataOfEvent.OperationType = opMODIFY;
                
                status = ProcessOperationAndAggregate(opMODIFY, NULL, &DataOfEvent, Watches, NumberOfWatches, HInotify, FileInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Failed to execute ProcessOperationAndAggregate() (MODIFY). \n");   
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }                     
            }

        } // --> while(1)
     


        // If here, everything is ok.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: BuildEventsForAllSubdirFiles(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: BuildEventsForAllSubdirFiles(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        if (NULL != dirStream)
        {
            closedir(dirStream);
            dirStream = NULL;
        }
    }
    else
    {
        if (NULL != dirStream)
        {
            closedir(dirStream);
            dirStream = NULL;
        }
    }

    return status;
} // BuildEventsForAllSubdirFiles()





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
    )
/*++
Description: The routine creates all the watches and the events necessary for monitoring and synchronizing a directory and all of its
content / files / subdirectories (this being usually the case of a MOVED_TO operation for a directory).
The concerned directory is represented by DirRelativePath, DirFullPath and DirName. EventWatchIndex identifies the watch that recorded
the event (usually MOVED_TO) of the directory. 

Note: The routine does not create an event for the first directory (DirRelativePath). But only for the inner files and directories.

- DirRelativePath: Pointer to the relative path of the directory.
- DirFullPath: Pointer to the full path of the directory.
- DirName: Pointer to the short name of the directory.
- EventWatchIndex: Index, in the Watches array, of the directory watch where the event (usually MOVED_TO) occured.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
- FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS status;
    DWORD indexOfNewDirWatch;

    // PREINIT.

    status = STATUS_FAIL;
    indexOfNewDirWatch = 0;

    // Parameter validation.

    if (NULL == DirRelativePath || 0 == DirRelativePath[0])
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == DirFullPath || 0 == DirFullPath[0])
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == DirName || 0 == DirName[0])
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 5.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 6.\n");
        return STATUS_FAIL;
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 7.\n");
        return STATUS_FAIL;
    }
    // The 6th parameter must be validated before the 4th.
    if ((*NumberOfWatches) <= EventWatchIndex)
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Invalid parameter 4.\n");
        return STATUS_FAIL;        
    }


    __try
    {
        // INIT.
        // --


        //
        // Main processing:
        //


        // Check the watch tree nodes for the existence of a previous directory with same path (possibly replaced by MOVE).
        // If so, a cleanup is performed.

        status = CheckWatchNodeExistenceForCleanup(DirName, (*Watches)[EventWatchIndex].TreeNode, (*Watches), NumberOfWatches, HInotify);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed to execute CheckWatchNodeExistenceForCleanup(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Create a directory watch for the first directory (DirFullPath).

        status = CreateDirWatchForDirectory(DirRelativePath, DirFullPath, NULL, Watches, NumberOfWatches, HInotify);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed to execute CreateDirWatchForDirectory(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Create a watch node for the first directory.

        indexOfNewDirWatch = (*NumberOfWatches) - 1;

        status = CreateWatchNode(indexOfNewDirWatch, (*Watches));
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed at CreateWatchNode().\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        status = AddChildWatchNodeToTree(FALSE, indexOfNewDirWatch, EventWatchIndex, DirName, (*Watches));
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed at AddChildWatchNodeToTree().\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }                    


        // Create watches for all subdirectories of DirFullPath (all depths subdirectories), but excluding DirFullPath.

        status = CreateWatchStructuresForAllSubdirectories(indexOfNewDirWatch, Watches, NumberOfWatches, HInotify);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed at CreateWatchStructuresForAllSubdirectories(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Build and emit events for the creation of all the files inside the DirFullPath (including subdirectories of all depths).
        // This excludes DirFullPath (i.e. an event is not created for the first directory).

        status = BuildEventsForAllSubdirFiles(indexOfNewDirWatch, Watches, NumberOfWatches, HInotify, FileInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Failed to execute BuildEventsForAllSubdirFiles(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // If here, everything is ok.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: CreateStructuresAndEventsForDirMovedToOnly(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // CreateStructuresAndEventsForDirMovedToOnly()




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
    )
/*++
Description: The routine takes care of all the necessary updates related to paths, required after the movement of a directory.
The directory is identified by FileInfoOfDirMoved, NewDirRelativePath, NewDirName and NewWatchNodeOfParentDir. 
The updates include changes in the watch tree, watch directories (Watches), and FileInfos (FileInfoHMap). The routine performs
the updates only for the marked FileInfo's, i.e. the FileInfo's whose "MovementCookie" field is equal to the MatchingCookie value.

- MatchingCookie: Integer representing the "movement cookie" which was priorly set in the FileInfo's affected by the directory movement.
- FileInfoOfDirMoved: Pointer to the FileInfo (of FileInfoHMap) associated with the moved directory.
- NewDirRelativePath: Pointer to the string containing the new relative path of the moved directory.
- NewDirName: Pointer to the string containing the short new filename of the moved directory.
- NewWatchNodeOfParentDir: Pointer to the new parent watch node of the moved directory.
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
- FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS            status;
    DIR_WATCH_NODE      *watchNodeOfMovedDir;
    DIR_WATCH_NODE      *oldWatchNodeOfParentDir;
    char                oldDirName[SD_MAX_FILENAME_LENGTH];
    char                relativePathBeforeMove[SD_MAX_PATH_LENGTH];
    char                symLinkFullPath[SD_MAX_PATH_LENGTH];
    BOOL                isFileInfoReplaced;
    BOOL                isSymLinkValid;
    FILE_INFO           *fileInfoToUpdate;
    std::string         auxString;

    // PREINIT.

    status = STATUS_FAIL;
    watchNodeOfMovedDir = NULL;
    oldWatchNodeOfParentDir = NULL;
    oldDirName[0] = 0;
    relativePathBeforeMove[0] = 0;
    symLinkFullPath[0] = 0;
    isFileInfoReplaced = FALSE;
    isSymLinkValid = FALSE;
    fileInfoToUpdate = NULL;

    // Parameter validation.


    if (0 == MatchingCookie)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == FileInfoOfDirMoved)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == NewDirRelativePath || 0 == NewDirRelativePath[0])
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    
    if (NULL == NewDirName || 0 == NewDirName[0])
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }
    if (NULL == NewWatchNodeOfParentDir)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 5.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 6.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 7.\n");
        return STATUS_FAIL;
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Invalid parameter 8.\n");
        return STATUS_FAIL;
    }



    __try
    {
        // INIT.

        // Get the node of the old parent directory and the old file name.

        oldWatchNodeOfParentDir = FileInfoOfDirMoved->WatchNodeOfParent;
        strcpy(oldDirName, FileInfoOfDirMoved->FileName);



        //
        // Main processing:
        //


        // Check the watch tree nodes for the existence of a previous directory with same path (possibly replaced by MOVE).
        // If so, a cleanup is performed.

        if (NewWatchNodeOfParentDir != oldWatchNodeOfParentDir)
        {        
            status = CheckWatchNodeExistenceForCleanup(NewDirName, NewWatchNodeOfParentDir, Watches, NumberOfWatches, HInotify);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to execute CheckWatchNodeExistenceForCleanup(). \n");   
                status = STATUS_FAIL;
                throw SyncDirException();
            }
        }


        // Get the watch node of the directory's old path.

        status = GetChildWatchNodeByDirName(oldWatchNodeOfParentDir, oldDirName, &watchNodeOfMovedDir);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to execute GetChildWatchNodeByDirName(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // 1 update for the watch tree:
        // - Cut & Paste the subtree of the directory's node (watchNodeOfMovedDir) - from old location to new one.
        // (old location: oldWatchNodeOfParentDir, new location: NewWatchNodeOfParentDir)

        if (NewWatchNodeOfParentDir != oldWatchNodeOfParentDir)
        {
            NewWatchNodeOfParentDir->Subdirs.push_back(watchNodeOfMovedDir);                    // New location.

            auto nodePos = std::find(oldWatchNodeOfParentDir->Subdirs.begin(), oldWatchNodeOfParentDir->Subdirs.end(), watchNodeOfMovedDir);
            if (oldWatchNodeOfParentDir->Subdirs.end() != nodePos)
            {
                oldWatchNodeOfParentDir->Subdirs.erase(nodePos);                                // Old location.
            }
            else
            {
                printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to erase watch node from old location. \n");   
                status = STATUS_FAIL;
                throw SyncDirException();                
            }

            // For multiple identical values, erase-find can be replaced with erase-remove:
            // Subdirs.erase(std::remove(Subdirs.begin(), Subdirs.end(), watchNodeOfMovedDir), Subdirs.end())
            // -> this is because std::remove does not erase! but overwrites the matching values, with the rest of the vector (by copying).
            // -> at each vector copy, one end vector position becomes obsolete, unused. This obsolete tail is returned by std::remove.
        }


        // Update dir name, parent and depth.

        watchNodeOfMovedDir->DirName = NewDirName;                                  // operator=(const char *).
        watchNodeOfMovedDir->Parent = NewWatchNodeOfParentDir;
        watchNodeOfMovedDir->Depth = NewWatchNodeOfParentDir->Depth + 1;


        // N updates for watches (N == number of directories):
        // - update the path of every watch directory in the subtree of watchNodeOfMovedDir.

        status = UpdatePathsForSubTreeWatches(watchNodeOfMovedDir, Watches);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to execute UpdatePathsForSubTreeWatches(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Update the paths in the FileInfo of the first directory: FileInfoOfDirMoved.

        status = UpdateFileInfoPath(FileInfoOfDirMoved, NewWatchNodeOfParentDir, NewDirName, NewDirRelativePath, Watches, NumberOfWatches,
            HInotify, FileInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to execute UpdateFileInfoPath(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }



        // Update the paths inside all the FileInfo's of the hash map FileInfoHMap.
        // If FileInfo matches the cookie for the MOVE operation, then:
        // 1. Update FileInfo path.
        // 2. Verify the integrity of a posible soft link and update the FileInfo.
        // 3. Reset the movement cookie.
        // 4. Insert the updated FileInfo, erase the old one.


        for (auto it = FileInfoHMap.begin(); it != FileInfoHMap.end(); )
        {
            isFileInfoReplaced = FALSE;                                     // Init.
            fileInfoToUpdate = & (it->second);                     


            if (MatchingCookie == fileInfoToUpdate->MovementCookie)
            {


                // 1.
                // Memorize the old relative file path for further usage.
                // Update the relative path in the FileInfo.

                strcpy(relativePathBeforeMove, fileInfoToUpdate->RelativePath);

                snprintf(fileInfoToUpdate->RelativePath, SD_MAX_PATH_LENGTH, "%s/%s", 
                    Watches[fileInfoToUpdate->WatchNodeOfParent->DirWatchIndex].DirRelativePath, fileInfoToUpdate->FileName);
                


                // 2.
                // Verify if soft links are still valid after changing paths.
                // If so, the real relative path is updated in the FileInfo (part of the IsSymbolicLinkValid function).
                // If not, do nothing. The link cannot be accessed anyway (i.e. broken link). 

                if (ftSYMLINK == fileInfoToUpdate->FileType)
                {                    
                    snprintf(symLinkFullPath, SD_MAX_PATH_LENGTH, "%s/%s", 
                        Watches[fileInfoToUpdate->WatchNodeOfParent->DirWatchIndex].DirFullPath, fileInfoToUpdate->FileName);

                    status = IsSymbolicLinkValid(symLinkFullPath, Watches[0].DirFullPath, &isSymLinkValid, fileInfoToUpdate->RealRelativePath);                       
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to execute IsSymbolicLinkValid(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                }



                // 3.
                // Unmark the FileInfo ("MOVE" terminated, reset cookie).

                fileInfoToUpdate->MovementCookie = 0;    // Important!



                // 4.
                // Erase the FileInfo for old key (path).
                // Insert the FileInfo for new key (path). 
                // i) insert before the erasure, otherwise the element would be destroyed before insertion.
                // ii) after insertion, the element is COPIED in the hash map ==> safe to erase.
                // iii) the strcmp() is needed just for safety. Because otherwise insert might just "update" and get erased.

                if (0 != strcmp(relativePathBeforeMove, fileInfoToUpdate->RelativePath))
                {
                    auxString.assign(fileInfoToUpdate->RelativePath);                           // Transform to std::string.

                    auto pair = FileInfoHMap.insert({auxString, (*fileInfoToUpdate)});
                    if (FALSE == pair.second)
                    {
                        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Failed to insert the updated FileInfo"
                            " in the hash map (after path update).\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                    it = FileInfoHMap.erase(it);
                    isFileInfoReplaced = TRUE;
                }



            } //--> if (MatchingCookie == fileInfoToUpdate->MovementCookie)


            if (FALSE == isFileInfoReplaced)    // Increment iterator, if not already done by the erasure "erase(it)".
            {
                ++it;
            }

        } //--> for (auto &element: FileInfoHMap)



        // If here, everything is ok.
        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: UpdatePathsByCookieForDirMovedFromAndTo(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // UpdatePathsByCookieForDirMovedFromAndTo()





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
    )
/*++
Description: Core of the SyncDir client application. Affects all structures of SyncDir system by itegrating events with one another
(aggregation) and records all changes in the Syncdir data structures. The routine processes a file operation (event), logs the resulted 
information and performs modifications that reflect all the effects of the event in the SyncDir application (client).

Note: The routine addresses exclusively the client part of SyncDir. It does not handle communications with the SyncDir server.
Note: The Event argument and the DataOfEvent argument are mutually excluding each other (1 == 'usage Event' XOR 'usage DataOfEvent). Hence,
if one of them is not provided (i.e. NULL), the other one must be.

- OperationType: Enumeration representing the type of the operation.
- Event: Mandatory if DataOfEvent is not used, NULL otherwise. Pointer to the structure containing the event information.
- DataOfEvent: Mandatory if Event is not used, NULL otherwise. Pointer to the structure containing the user-defined event data.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
- FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    DWORD           eventWatchIndex;
    DWORD           eventCookie;
    BOOL            isSymLinkValid;
    BOOL            isFileStillAccessible;
    BOOL            eventIsForDirectory;
    FILE_INFO       newFileInfo;    
    FILE_INFO       *fileInfoToAggregate;
    FILE_INFO       *fileInfoWithCookie;
    DIR_WATCH_NODE  *eventWatchNode;
    DIR_WATCH_NODE  *dirNodeToDelete;
    char            eventRelativePath[SD_MAX_PATH_LENGTH];
    char            eventFullPath[SD_MAX_PATH_LENGTH];
    char            eventFileName[SD_MAX_FILENAME_LENGTH];
    struct stat     eventFileStat;
    std::string     auxString;
    std::unordered_map<std::string, FILE_INFO>::iterator iteratorFI;

    // PREINIT.

    status = STATUS_FAIL;
    eventWatchIndex = 0;
    eventCookie = 0;
    isSymLinkValid = FALSE;
    isFileStillAccessible = FALSE;
    eventIsForDirectory = FALSE;
    fileInfoToAggregate = NULL;
    fileInfoWithCookie = NULL;
    eventWatchNode = NULL;
    dirNodeToDelete = NULL;
    eventRelativePath[0] = 0;
    eventFullPath[0] = 0;
    eventFileName[0] = 0;

    // Parameter validation.

    if (opUNKNOWN == OperationType)
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == Event && NULL == DataOfEvent)
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Condition invalidated for parameter 2 and 3 (cannot be both NULL). \n");
        return STATUS_FAIL;
    }
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Invalid parameter 5.\n");
        return STATUS_FAIL;
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Invalid parameter 6.\n");
        return STATUS_FAIL;
    }


    __try
    {

        // *** INIT *** (start).



        // Init I. 
        // Initialize:
        // - the watch index of the event.
        // - the paths of the event file.
        // - the event cookie value.
        // - the dir/non-dir indicator.
        // Use DataOfEvent ? or Event ? Check validity:
        
        if (NULL != DataOfEvent)                                            // Initialize local variables using DataOfEvent.
        {
            eventWatchIndex = DataOfEvent->WatchIndex;                                  
            eventIsForDirectory = DataOfEvent->IsDirectory;
            eventCookie = DataOfEvent->Cookie;
            strcpy(eventRelativePath, DataOfEvent->RelativePath);
            strcpy(eventFullPath, DataOfEvent->FullPath);
            strcpy(eventFileName, DataOfEvent->FileName);            
        }
        else if (NULL != Event)                                             // Initialize local variables using inotify_event structure.
        {
            status = GetDirWatchIndexByHandle(Event->wd, (*Watches), (*NumberOfWatches), &eventWatchIndex);   
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute GetDirWatchIndexByHandle(). \n");   
                status = STATUS_FAIL;
                throw SyncDirException();
            }

            eventCookie = Event->cookie;
            snprintf(eventRelativePath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[eventWatchIndex].DirRelativePath,  Event->name);            
            snprintf(eventFullPath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[eventWatchIndex].DirFullPath, Event->name); 
            strcpy(eventFileName, Event->name);            

            if (IN_ISDIR & Event->mask)                                                         // Check if dir or non-dir.
            {
                eventIsForDirectory = TRUE;        
            }
            else
            {
                eventIsForDirectory = FALSE;
            }
        }



        // Brief info printed at stdout.
        if (TRUE == eventIsForDirectory)
        {
            printf("[SyncDir] Info: Event for directory [%s] in the path at [%s]:\n", eventFileName, (*Watches)[eventWatchIndex].DirRelativePath);
        }
        else
        {
            printf("[SyncDir] Info: Event for file [%s] in the path at [%s]:\n", eventFileName, (*Watches)[eventWatchIndex].DirRelativePath);
        }



        // Init II. 
        // Get the tree node of the watch that captured the event.

        eventWatchNode = (*Watches)[eventWatchIndex].TreeNode;


        
        // Init III. 
        // Get information on the event file, using the stat structure eventFileStat.

        if (lstat(eventFullPath, &eventFileStat) < 0)
        {
            perror("[SyncDir] Info: ProcessOperationAndAggregate(): Could not execute lstat(). \n");
            printf("lstat() info was for file [%s]. \n", eventFullPath);
            printf("File may not exist anymore, or the user renamed/moved the file meanwhile. \n");

            isFileStillAccessible = FALSE;
        }
        else
        {
            isFileStillAccessible = TRUE;
        }



        // Init IV.
        // Initialize fileInfoToAggregate.
        // -> See if there exists already a FileInfo for the event file (event path).
        // -> If so, the routine main processing will aggregate the events/operations in the same FileInfo.

        auxString.assign(eventRelativePath);                                // Transform to std::string.

        iteratorFI = FileInfoHMap.find(auxString);
        if (FileInfoHMap.end() == iteratorFI)
        {
            fileInfoToAggregate = NULL;                                    // NO events registered for the file.
        }
        else
        {
            fileInfoToAggregate =  &iteratorFI->second;                    // Events already registered for the file.
        }



        // Init V.
        // Initialize a new FileInfo for the event file. 
        // The new FileInfo will be updated and inserted in the FileInfoHMap, in case there is no FileInfo already for the event file.

        status = InitFileInfo(&newFileInfo);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute InitFileInfo(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }        
        strcpy(newFileInfo.RelativePath, eventRelativePath);
        strcpy(newFileInfo.FileName, eventFileName);
        newFileInfo.WatchNodeOfParent = eventWatchNode;

        if (TRUE == eventIsForDirectory)
        {
            newFileInfo.FileType = ftDIRECTORY;
        }
        else
        {
            newFileInfo.FileType = ftNONDIR;
        }



        // Init VI.
        // Verify symbolic link injection: 
        // Check whether or not the event file points inside the main directory.
        // -> If the symbolic link is validated ==> set the ftSYMLINK flag and the real relative path.
        // -> If the symbolic link points outside of the main directory ==> Terminate processing (skip this operation/event).        

        if ((TRUE == isFileStillAccessible) && (S_ISLNK(eventFileStat.st_mode)))
        {
            status = IsSymbolicLinkValid(eventFullPath, (*Watches)[0].DirFullPath, &isSymLinkValid, newFileInfo.RealRelativePath);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute IsSymbolicLinkValid(). \n");   
                status = STATUS_WARNING;
                // Only warning because the file may not exist anymore, or it's a broken link.
            }            

            if (TRUE == isSymLinkValid && STATUS_SUCCESS == status)
            {
                newFileInfo.FileType = ftSYMLINK;                          

                if (NULL != fileInfoToAggregate)
                {
                    fileInfoToAggregate->FileType = ftSYMLINK;
                    strcpy(fileInfoToAggregate->RealRelativePath, newFileInfo.RealRelativePath);
                }
            }
            else if (STATUS_SUCCESS == status)
            {
                printf("[SyncDir] Warning: The symbolic link points outside of the main folder. Ending processing the operation.\n");
                status = STATUS_WARNING;                                // WARNING only, because it is a normal event (skipped).
                throw SyncDirException();                               // Throw, to terminate. But warning because of the symlink.
            }
        }


        // Log.
        if (NULL != fileInfoToAggregate)
        {
            fprintf(g_SD_STDLOG, "[SyncDir] Info: Existent file record (FileInfo) found. Using FileInfo for aggregation. \n");
        }
        else
        {
            fprintf(g_SD_STDLOG, "[SyncDir] Info: No FileInfo record found. Creating new FileInfo. \n");
        }
        

        // --> *** INIT *** (end).
        


        //
        // Start main processing:
        // 


        //
        // OPERATION LOGGING & SYNCDIR UPDATE (client data structures)
        //

        // This part is the core of the SyncDir process. 
        //
        // - Operation logging and aggregation are performed (for future server update). 
        // - SyncDir data structure updates are performed (to reflect the Inotify events throughout SyncDir).
        //
        // Note: Aggregation of new and old events information is made based on an already existing FileInfo.
        // ==> The new events will reflect in the updated FileInfo.        
        // Note: An already existing FileInfo (for the event file) is indicated by fileInfoToAggregate (if not NULL).
        // ==> A fileInfoToAggregate not NULL means that logged events already exist for the event file (eventRelativePath).


        switch (OperationType)                                              // Process operation.
        {


            //
            // DIR_DELETE
            // FIL_DELETE
            //

            case (opDIRDELETE):
            case (opFILDELETE):
            case (opDELETE):            


                //
                // FileInfo: NO ==> Insert new.
                //
                if (NULL == fileInfoToAggregate)                        // If no events registered so far (for the event file).
                {
                    newFileInfo.FileExistedBeforeEvents = TRUE;
                    newFileInfo.WasDeleted = TRUE;

                    status = InsertNewFileInfo(&newFileInfo, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute InsertNewFileInfo() (DELETE). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                                
                }


                //
                // FileInfo: YES ==> Aggregate events.
                //
                if (NULL != fileInfoToAggregate)                        // If events registered already (for the event file).
                {
                    fileInfoToAggregate->WasDeleted = TRUE;
                }


                // Aggregate events (completion):
                // - delete all FileInfo's that are related to the deleted directory (excluding the dir's own FileInfo, of above).
                // Delete all watch-related structures of the deleted directory:
                // - watch node subtree (DIR_WATCH_NODE's);
                // - watch directories (DIR_WATCH's);
                // - watch descriptors (of Inotify);

                if (TRUE == eventIsForDirectory)
                {
                    status = DeleteAllFileInfosForDir(eventRelativePath, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute DeleteAllFileInfosForDir(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                                                    
                    status = GetChildWatchNodeByDirName(eventWatchNode, eventFileName, &dirNodeToDelete);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute GetChildWatchNodeByDirName(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                                                    
                    status = DeleteWatchesAndNodesOfSubdirs(dirNodeToDelete, (*Watches), NumberOfWatches, HInotify);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute DeleteWatchesAndNodesOfSubdirs(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                                                    
                }


                break;



            // ***********************************************************
            //
            // DIR_MOVED_FROM
            // FIL_MOVED_FROM
            // MOVED_FROM
            //

            case (opDIRMOVEDFROM):
            case (opFILMOVEDFROM):
            case (opMOVEDFROM): 

                
                //
                // FileInfo: NO ==> Insert new.
                //
                if (NULL == fileInfoToAggregate)
                {
                    newFileInfo.FileExistedBeforeEvents = TRUE;
                    newFileInfo.WasMovedFromOnly = TRUE;
                    newFileInfo.WasDeleted = FALSE;
                    newFileInfo.WasMovedFromAndTo = FALSE;
                    newFileInfo.MovementCookie = eventCookie;
                    
                    status = InsertNewFileInfo(&newFileInfo, FileInfoHMap);           
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute InsertNewFileInfo() (MOVED_FROM). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }


                //
                // FileInfo: YES ==> Aggregate events.
                //
                if (NULL != fileInfoToAggregate)
                {
                    fileInfoToAggregate->WasMovedFromOnly = TRUE;
                    fileInfoToAggregate->WasDeleted = FALSE;
                    fileInfoToAggregate->WasMovedFromAndTo = FALSE;
                    fileInfoToAggregate->MovementCookie = eventCookie;
                }


                // Aggregate events (completion):
                // If directory, then mark with a cookie all related FileInfos (for future movement or deletion).
                // A future possible MOVED_TO operation, that has the same cookie value associated, will conclude these related FileInfo's
                // as part of a movement operation. If no associated MOVED_TO will be present, the FileInfo's will be finally deleted.

                if (TRUE == eventIsForDirectory)
                {
                    status = SetMovementCookiesForDirMovedFrom(eventCookie, eventRelativePath, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute SetMovementCookiesForDirMovedFrom(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }


                break;             



            // ***********************************************************
            //
            // DIR_MOVED_TO
            // DIR_MOVE
            //

            case (opDIRMOVEDTO):


                // Find the FileInfo whose cookie value matches the current event cookie (i.e. try to match the MOVED_TO to a 
                // previous MOVED_FROM operation).

                status = FindFileInfoByMovementCookie(eventCookie, &fileInfoWithCookie, FileInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute FindFileInfoByMovementCookie() (DIR). \n");   
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }



                // Cookie: not found ==> MOVED_TO operation.
                // - CreateStructuresAndEventsForDirMovedToOnly(): 
                // Create all the necessary structures to integrate the new directory (and all its contents) in the SyncDir system:
                // Inotify watches, directory watches (DIR_WATCH), watch node subtree (DIR_WATCH_NODE), creation events, operation logging
                // for server update (FILE_INFO) -> event aggregation included.
                // - Goto is used to handle the "create" operation of the current event directory ==> hence, jump to opCREATE.

                if (NULL == fileInfoWithCookie)
                {
                    //
                    // DIR_MOVED_TO
                    //

                    status = CreateStructuresAndEventsForDirMovedToOnly(eventRelativePath, eventFullPath, eventFileName, eventWatchIndex,
                        Watches, NumberOfWatches, HInotify, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed at CreateStructuresAndEventsForDirMovedToOnly() "
                            "called at DIRMOVEDTO. \n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }

                    newFileInfo.FileExistedBeforeEvents = TRUE;             // TRUE, since it might have replaced an existing dir.
                    goto label_opCREATE;                                    // goto ==> same process as opDIRCREATE;
                }



                // Cookie: found ==> MOVE operation.
                // - FileInfo: YES (fileInfoWithCookie) ==> Aggregate events.
                // - Effects of the DIR_MOVE operation (part of event aggregation):
                //      -> Change paths in all SyncDir structures (including FileInfo's whose files are in the presently moved directory).

                if (NULL != fileInfoWithCookie)
                {
                    //
                    // DIR_MOVE
                    //

                    fileInfoToAggregate = fileInfoWithCookie;                

                    fileInfoToAggregate->WasMovedFromAndTo = TRUE;
                    fileInfoToAggregate->WasDeleted = FALSE;
                    fileInfoToAggregate->WasMovedFromOnly = FALSE;

                    status = UpdatePathsByCookieForDirMovedFromAndTo(eventCookie, fileInfoToAggregate, eventRelativePath, eventFileName, 
                        eventWatchNode, (*Watches), NumberOfWatches, HInotify, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed at UpdatePathsByCookieForDirMovedFromAndTo(). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                        
                }


                break;                



            // ***********************************************************
            //
            // FIL_MOVED_TO
            // FIL_MOVE
            //

            case (opFILMOVEDTO):


                // Find the FileInfo whose cookie value matches the current event cookie (i.e. try to match the MOVED_TO to a 
                // previous MOVED_FROM operation).

                status = FindFileInfoByMovementCookie(eventCookie, &fileInfoWithCookie, FileInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute FindFileInfoByMovementCookie() (FIL). \n");   
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }



                // Cookie: not found ==> MOVED_TO operation.
                // - Goto is used to handle the FIL_MOVED_TO as a MODIFY operation (appropriate, due to same operation effects).

                if (NULL == fileInfoWithCookie)
                {
                    //
                    // FIL_MOVED_TO
                    // 

                    newFileInfo.FileExistedBeforeEvents = TRUE;             // TRUE, because it might have replaced an existing file.
                    goto label_opMODIFY;                                    // goto ==> same process as MODIFY.
                }



                // Cookie: found ==> MOVE operation.
                // - FileInfo: YES (fileInfoWithCookie) ==> Aggregate events.
                // - Effects of the FIL_MOVE operation (part of event aggregation): Change path of the FileInfo being aggregated.

                if (NULL != fileInfoWithCookie)
                {
                    //
                    // FIL_MOVE
                    //

                    fileInfoToAggregate = fileInfoWithCookie;

                    fileInfoToAggregate->WasMovedFromAndTo = TRUE;
                    fileInfoToAggregate->WasDeleted = FALSE;
                    fileInfoToAggregate->WasMovedFromOnly = FALSE;  

                    status = UpdateFileInfoPath(fileInfoToAggregate, eventWatchNode, eventFileName, eventRelativePath, (*Watches),
                        NumberOfWatches, HInotify, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute UpdateFileInfoPath() (FIL_MOVE). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                }


                break;



            // ***********************************************************
            //
            // MODIFY
            //

            // FileExistedBeforeEvents must be set to TRUE here, since, if there are no logged events for the file, then
            //      in all situations a MODIFY operation should indicate that the file that was modified existed (at that path) 
            //      before the events. Even if the MODIFY is induced by another operation (e.g. MOVED_TO).
            //      Exception: when DataOfEvent is provided.
            // 
            // Example: "mv file_outside main_dir/file_inside ; rm main_dir/file_inside" ==
            //      == Using "mv" to delete existing files (inside main_dir of SyncDir), using outside files (not existing before).
            //      

            case (opMODIFY):            
            label_opMODIFY:                                                     // label used for redirecting (jump from) an opFILMOVEDTO.


                //
                // FileInfo: NO ==> Insert new.
                //
                if (NULL == fileInfoToAggregate)
                {
                    newFileInfo.FileExistedBeforeEvents = (NULL != DataOfEvent) ? DataOfEvent->FileExistedBeforeEvents : TRUE;
                    newFileInfo.WasModified = TRUE;
                    newFileInfo.WasDeleted = FALSE;
                    newFileInfo.WasMovedFromOnly = FALSE;
                    newFileInfo.WasMovedFromAndTo = FALSE;       
                    //newFileInfo.WasMovedToOnly = FALSE; // Deprecated.  

                    status = InsertNewFileInfo(&newFileInfo, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute InsertNewFileInfo() (MODIFY). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                }


                //
                // FileInfo: YES ==> Aggregate events.
                //
                if (NULL != fileInfoToAggregate)
                {
                    fileInfoToAggregate->WasModified = TRUE;
                    fileInfoToAggregate->WasDeleted = FALSE;
                    fileInfoToAggregate->WasMovedFromOnly = FALSE;              // Because file exists now inside the main dir (SyncDir space).
                                                                                // Regardless of the previous events.
                }

                break;



            // ***********************************************************
            // 
            // DIR_CREATE
            // FIL_CREATE
            // CREATE
            //

            // FileExistedBeforeEvents is set to TRUE here only at a redirection of a DIR_MOVED_TO operation. The motivation is that
            // the "moved to" directory might have replaced an existing one.

            case (opDIRCREATE):
            case (opFILCREATE):
            case (opCREATE):
            label_opCREATE:                                                 // label used for redirecting (jump from) an opDIRMOVEDTO.


                //
                // FileInfo: NO ==> Insert new.
                // 
                if (NULL == fileInfoToAggregate)
                {
                    newFileInfo.FileExistedBeforeEvents =  (opDIRMOVEDTO == OperationType) ? TRUE : FALSE;
                    newFileInfo.WasCreated = TRUE;
                    newFileInfo.WasDeleted = FALSE;
                    newFileInfo.WasMovedFromOnly = FALSE;
                    //newFileInfo.WasMovedToOnly = FALSE; // Deprecated.
                    newFileInfo.WasMovedFromAndTo = FALSE;  
                    newFileInfo.WasModified = FALSE;

                    status = InsertNewFileInfo(&newFileInfo, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed to execute InsertNewFileInfo() (CREATE). \n");   
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }


                //
                // FileInfo: YES ==> Aggregate events.
                //
                if (NULL != fileInfoToAggregate)
                {
                    fileInfoToAggregate->WasCreated = TRUE;
                    fileInfoToAggregate->WasDeleted = FALSE;
                    fileInfoToAggregate->WasMovedFromOnly = FALSE;
                    //fileInfoToAggregate->WasMovedToOnly = FALSE; // Deprecated.
                    fileInfoToAggregate->WasMovedFromAndTo = FALSE;  
                    fileInfoToAggregate->WasModified = FALSE;                    
                }


                // Create the directory. (If it's a pure CREATE operation, not coming from a redirected DIRMOVEDTO already treated.)

                if (TRUE == eventIsForDirectory && opDIRMOVEDTO != OperationType)
                {
                    status = CreateStructuresAndEventsForDirMovedToOnly(eventRelativePath, eventFullPath, eventFileName, eventWatchIndex,
                        Watches, NumberOfWatches, HInotify, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Failed at CreateStructuresAndEventsForDirMovedToOnly() "
                            "called at CREATE. \n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }


                break;        



            default:

                printf("[SyncDir] Error: ProcessOperationAndAggregate(): Consistency error at the Operation Type.\n");
                status = STATUS_FAIL;
                throw SyncDirException();   



        } // --> switch(OperationType)


        // If here, everything worked well.
        status = SUCCESS_KEEP_WARNING(status);

    } // __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: ProcessOperationAndAggregate(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: ProcessOperationAndAggregate(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // ProcessOperationAndAggregate()




//
// ReadEventsAndIdentifyOperations
//
SDSTATUS
ReadEventsAndIdentifyOperations(
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __inout unordered_map <std::string, FILE_INFO> &FileInfoHMap
    )
/*++
Description: The routine reads all the events from the Inotify handle HInotify, identifies the operations associated
with each event and passes the operations further for processing and logging in the map of FileInfoHMap. For the latter
processing, the routine of ProcessOperationAndAggregate() is used.

- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).
- FileInfoHMap: Reference to the hash map of FILE_INFO structures generated by file events.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS                status;
    __int32                 readBytes;
    DWORD                   crtEventPosition;
    struct inotify_event    *event;
    char                    eventBuffer[SD_EVENT_BUFFER_SIZE];
    OP_TYPE                 operationType;    
    
    // PREINIT.

    status = STATUS_FAIL;
    readBytes = -1;    
    event = NULL;
    eventBuffer[0] = 0;
    crtEventPosition = 0;
    operationType = opUNKNOWN;

    // Parameter validation.    

    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }    
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    


    __try
    {
        // INIT.
        // --


        // 
        // Main processing:
        //


        //
        // LOOP READING EVENTS:
        //

        // Extract at most SD_EVENT_BUFFER_SIZE bytes of events from the Inotify queue.
        //
        // Loop condition (at the end):
        // If the buffer sized is reached (SD_EVENT_BUFFER_SIZE), the loop continues to read and process events. Otherwise, exit.

        while (1)
        {

            readBytes = read(HInotify, eventBuffer, SD_EVENT_BUFFER_SIZE);
            if (readBytes < 0)
            {
                perror("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Could not read from watch descriptor.\n");
                status = STATUS_FAIL;
                throw SyncDirException();
            }
            if (0 == readBytes)
            {
                printf("[SyncDir] Info: Event buffer empty. EOF was read from Inotify handle (0 bytes of data).\n");
                break;
            }
            printf("[SyncDir] Info: Block of events read.\n");

            

            //
            // LOOP PARSING EVENTS (==> Operations) :
            //

            // Process events one after another until the end of the event buffer.
            // For every event, the associated operation is identified and passed on for logging.
            // An operation is identified by flags in the mask of the event:
            // - IN_ISDIR
            // - IN_CREATE, IN_DELETE, IN_MOVED_FROM, IN_MOVED_TO, IN_MODIFY

            crtEventPosition = 0;
            while (crtEventPosition < (DWORD)readBytes)
            {
                event = (struct inotify_event *) &eventBuffer[crtEventPosition];        // Cast and extract the event at current offset.



                if (0 < event->len)                                                     // Continue only if not a "NULL name event".
                {
                    operationType = opUNKNOWN;


                    // Identify operation:

                    if (IN_CREATE & event->mask)
                    {
                        if (IN_ISDIR & event->mask)
                        {
                            printf("\n-- CREATE directory --\n");
                            operationType = opDIRCREATE;
                        }
                        else
                        {
                            printf("\n-- CREATE file --\n");
                            operationType = opFILCREATE;
                        }
                    }
                    if (IN_DELETE & event->mask)
                    {
                        if (IN_ISDIR & event->mask)
                        {
                            printf("\n-- DELETE directory --\n");
                            operationType = opDIRDELETE;
                        }
                        else
                        {
                            printf("\n-- DELETE file --\n");
                            operationType = opFILDELETE;
                        }
                    }
                    if (IN_MOVED_FROM & event->mask)
                    {
                        if (IN_ISDIR & event->mask)
                        {
                            printf("\n-- Directory MOVED_FROM --\n");
                            operationType = opDIRMOVEDFROM;
                        }
                        else
                        {
                            printf("\n-- File MOVED_FROM --\n");
                            operationType = opFILMOVEDFROM;
                        }
                    }
                    if (IN_MOVED_TO & event->mask)
                    {
                        if (IN_ISDIR & event->mask)
                        {
                            printf("\n-- Directory MOVED_TO --\n");
                            operationType = opDIRMOVEDTO;
                        }
                        else
                        {
                            printf("\n-- File MOVED_TO --\n");
                            operationType = opFILMOVEDTO;
                        }
                    }
                    if (IN_MODIFY & event->mask)
                    {
                        printf("\n-- MODIFY file --\n");    
                        operationType = opMODIFY;
                    }
                    if (opUNKNOWN == operationType)                 // If operation not identified, log error.
                    {
                        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Operation type UNKNOWN.\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }


                    // Transmit the operation for processing and aggregation.

                    status = ProcessOperationAndAggregate(operationType, event, NULL, Watches, NumberOfWatches, HInotify, FileInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Failed to execute ProcessOperationAndAggregate().\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                } // --> if (event->len)



                // Jump to next event in the buffer:
                // - add the size of event (SD_EVENT_SIZE) --> does not include the variable string "event->name".
                // - add the length of event->name field (event->len) --> includes the struct padding as well.

                crtEventPosition = crtEventPosition + SD_EVENT_SIZE + event->len;

            } //--> while (crtEventPosition < (DWORD)readBytes)



            // Exit the event processing, if all data was read.
            // BECAUSE if not, read() would block undefinitely (until new event appears).
            if ((unsigned)readBytes < SD_EVENT_BUFFER_SIZE)
            {
                printf("[SyncDir] Info: Exiting. Reached END of event buffer.\n");
                break;
            }
        } // --> while(1)



        // If here, everything worked well.
        status = STATUS_SUCCESS;

    } // __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "ReadEventsAndIdentifyOperations(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: ReadEventsAndIdentifyOperations(): Unkown exception.\n");
        status = STATUS_FAIL;
    }

    // Cleanup. UNINIT.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // ReadEventsAndIdentifyOperations().



//
// WaitForEventsAndProcessChanges
//
SDSTATUS
WaitForEventsAndProcessChanges(
    __in char           *MainDirFullPath,
    __inout DIR_WATCH   **Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify,
    __in __int32        CltSock
    )
/*++
Description: The routine waits for events to appear on SyncDir client side directories, then transmits the events for processing 
and logging, and later decides when to transfer the changes (operations) to the SyncDir server.
The routine can wait for events an infinite amount of time, or a limited one (if set so, by the user, at SyncDir launch).
Before sending the changes to server, the routine performs a last check of the HInotify kernel queue, thus aggregating any
collateral events that may meanwhile appear.

- MainDirFullPath: Pointer to the string containg the full (absolute) path towards the main directory monitored by SyncDir client application.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches (which monitor the directories).

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING could be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    __int32         existEventsToRead;
    QWORD           elapsedTime;
    fd_set          readableFdSet;
    struct timeval  waitOnSelect;
    struct timeval  startTime;
    struct timeval  crtTime;
    BYTE            isMinTimeBeforeSyncActive;
    std::unordered_map<std::string, FILE_INFO> fileInfoHMap;

    // PREINIT.

    status = STATUS_FAIL;
    existEventsToRead = -1;
    elapsedTime = 0;
    isMinTimeBeforeSyncActive = 1;

    // Validate parameters.

    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.


        // Clear the file descriptor set; initialize the time structures;
        // seed the random generator; 

        FD_ZERO(&readableFdSet);
        waitOnSelect.tv_sec = 0;
        waitOnSelect.tv_usec = 0;
        elapsedTime = 0;                
        if ( (-1 == gettimeofday(&startTime, NULL)) || (-1 == gettimeofday(&crtTime, NULL)) )
        {
            perror("[SyncDir] Error: WaitForEventsAndProcessChanges(): gettimeofday() failed.\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        srand(startTime.tv_sec);        
        fileInfoHMap.clear();


        // Build and emit events for the creation of all the files inside the main directory (including subdirectories of all depths).
        // This excludes the main directory (i.e. an event is not created for the first directory).

        status = BuildEventsForAllSubdirFiles(0, Watches, NumberOfWatches, HInotify, fileInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Failed to execute BuildEventsForAllSubdirFiles(). \n");   
            status = STATUS_FAIL;
            throw SyncDirException();
        }    


        fprintf(g_SD_STDLOG, "[SyncDir] Info: All the events were built for the current state of the client partition. \n");


        // Inform the server. Synchronize the current partition state. 

        status = SendAllFileInfoEventsToServer(MainDirFullPath, fileInfoHMap, CltSock);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Failed to execute SendAllFileInfoEventsToServer() (at INIT).\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        
        fprintf(g_SD_STDLOG, "[SyncDir] Info: Events of the current partition state were sent to the server. \n");


        // --> end of INIT.



        //
        // Start main processing.
        //


        //
        // LOOP MONITORING:
        //

        // Loop the waiting and processing of new events (plus updating the server).
        // -> Until a time limit is reached. This could be infinite, if user specified so.
        // I. Initialization.
        // II. Wait for events.
        // III. Loop event processing (+ server update at exit).
        // IV. Compute elapsed time.



        while(elapsedTime < gTimeLimit)
        {

            // I.
            // Clear the file descriptor set and enable HInotify descriptor.
            // Enable the usage of SD_MIN_TIME_BEFORE_SYNC.

            FD_ZERO(&readableFdSet);
            FD_SET(HInotify, &readableFdSet);

            isMinTimeBeforeSyncActive = 1;



            // II.
            // Wait until an event appears.
            // Wait undefinitely: select() with last argument NULL (waiting time).
            // -> select() is triggered for any descriptor in readableFdSet less than HInotify + 1.

            printf("[SyncDir] Info: Waiting for events ...\n\n");

            existEventsToRead = select(HInotify + 1, &readableFdSet, NULL, NULL, NULL);      
            if (-1 == existEventsToRead)
            {
                perror("[SyncDir] Error: WaitForEventsAndProcessChanges(): select() failed (first events).\n");
                status = STATUS_FAIL;
                throw SyncDirException();                
            }

            printf("*** Select unblocked. Out. ***\n");
            printf("[SyncDir] Info: Found events to read (1st Inotify answer). Reading events.\n");



            //
            // III. LOOP Event Processing & Server Update:
            //

            // III.A. Read, process and log all events. Update SyncDir data.
            // III.B. Check for any last events in the Inotify kernel queue.
            // III.C. If no events, update the server and exit. Otherwise, loop.

            while(1)
            {


                // III.A.
                // Identify and process operations associated to events. Log changes and update SyncDir data structures (client).

                if (FD_ISSET(HInotify, &readableFdSet))                 // Validate the HInotify.
                {
                    status = ReadEventsAndIdentifyOperations(Watches, NumberOfWatches, HInotify, fileInfoHMap);   
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Failed to execute "
                            "ReadEventsAndIdentifyOperations().\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }
                }
                else                                                    // Consistency error.
                {
                    printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Consistency error at select().\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }



                // III.B.
                // Wait (sleep) a limited number of seconds for any additional (associated) events that may appear.

                printf("[SyncDir] Info: ... Waiting few seconds (before updating the server) ...\n");
                sleep( ((QWORD)SD_MIN_TIME_BEFORE_SYNC * isMinTimeBeforeSyncActive) + (rand() % SD_TIME_TRESHOLD_AT_SYNC) );



                // III.C.
                // Perform a last instant check for events before updating server (0 == waiting time).
                // When waitOnSelect is 0, select() verifies instantly and exits.
                // - If no events ==> update server.
                // - If events still exist ==> loop for processing.

                FD_ZERO(&readableFdSet);                                // Init for select().
                FD_SET(HInotify, &readableFdSet);
                waitOnSelect.tv_sec = 0;
                waitOnSelect.tv_usec = 0;

                existEventsToRead = select(HInotify + 1, &readableFdSet, NULL, NULL, &waitOnSelect);
                if (-1 == existEventsToRead)                            // ==> Error.
                {
                    perror("[SyncDir] Error: WaitForEventsAndProcessChanges(): Instant select() failed.\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();                
                }
                if (0 == existEventsToRead)                              // ==> There are no events to be read. Update server.
                {
                    printf("[SyncDir] Info: No events left in the Event Queue. Sending data to the server.\n");

                    status = SendAllFileInfoEventsToServer(MainDirFullPath, fileInfoHMap, CltSock);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Failed to execute SendAllFileInfoEventsToServer().\n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }                    
                    break;
                }


                printf("[SyncDir] Info: Still events in the queue ...\n");  // Loop the event processing.

                isMinTimeBeforeSyncActive = 0;                              // Minimum time before sync has passed ==> Disable.



            } // --> while(1)



            // IV.
            // Compute elapsed time to know if the monitoring ends.

            if (-1 == gettimeofday(&crtTime, NULL))
            {
                perror("[SyncDir] Error: WaitForEventsAndProcessChanges(): Could not get the time of the day.\n");
                status = STATUS_FAIL;
                throw SyncDirException();
            }        

            elapsedTime = crtTime.tv_sec - startTime.tv_sec;
            printf("[SyncDir] Info: Reading the Event Queue finished. Elapsed time ... [%lu] seconds.\n", elapsedTime);


        } // --> while(elapsedTime < gTimeLimit)



        // If here, everything worked well.
        status = STATUS_SUCCESS;

    } // __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "WaitForEventsAndProcessChanges(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: WaitForEventsAndProcessChanges(): Unkown exception.\n");
        status = STATUS_FAIL;
    }

    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for now.        
    }
    else
    {
        // Nothing to clean for now.        
    }

    return status;
} // WaitForEventsAndProcessChanges().



