
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/



#include "syncdir_clt_watch_manager.h"

QWORD gWatchesArrayCapacity = SD_INITIAL_NR_OF_WATCHES;              // Only definition here.




//
// DeleteDirWatchByIndex
//
SDSTATUS
DeleteDirWatchByIndex(
    __in DWORD          DelIndex,
    __inout DIR_WATCH   *Watches,
    __inout DWORD       *NumberOfWatches,
    __in __int32        HInotify
    )
/*++
Description: The routine removes a directory watch (DIR_WATCH structure) from a provided array (Watches) and also removes its
associated Inotify watch from the provided Inotify instance (HInotify).

- DelIndex: Index in the Watches array, representing the directory watch to be removed.
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    __int32 hWatchToRemove;

    // PREINIT.

    status = STATUS_FAIL;
    hWatchToRemove = -1;

    // Parameter validation.

    // Start with parameter 2. Need to verify the 3rd before the 1st.
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: DeleteDirWatchByIndex(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: DeleteDirWatchByIndex(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: DeleteDirWatchByIndex(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }    
    if ((*NumberOfWatches) <= DelIndex)
    {
        printf("[SyncDir] Error: DeleteDirWatchByIndex(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }


    // INIT.

    hWatchToRemove = Watches[DelIndex].HWatch;


    // Main processing:

    // Delete directory watch at delIndex:
    // Replace the delIndex element with the last element of the array.

    Watches[DelIndex].HWatch = Watches[(*NumberOfWatches)-1].HWatch;
    strcpy(Watches[DelIndex].DirFullPath, Watches[(*NumberOfWatches)-1].DirFullPath);
    strcpy(Watches[DelIndex].DirRelativePath, Watches[(*NumberOfWatches)-1].DirRelativePath);

    Watches[(*NumberOfWatches)-1].HWatch = -1;
    Watches[(*NumberOfWatches)-1].DirFullPath[0] = 0;
    Watches[(*NumberOfWatches)-1].DirRelativePath[0] = 0;

    (*NumberOfWatches) = (*NumberOfWatches) - 1;

    // Remove the Inotify watch.

    if (inotify_rm_watch(HInotify, hWatchToRemove) < 0)
    {
        perror("[SyncDir] Warning: DeleteDirWatchByIndex(): inotify_rm_watch() did not succeed.\n");
        status = STATUS_WARNING;
    }


    // If here, everything worked well.
    status = SUCCESS_KEEP_WARNING(status);                                  // Returns a success status, including warning (if case).


    // UNINIT. Cleanup.
    //cleanup_DeleteDirWatchByIndex:

    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;

} // DeleteDirWatchByIndex()



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
    )
/*++
Description: The routine creates an Inotify watch and a directory watch (DIR_WATCH) for a given directory path.
Concretely, the routine uses the Inotify instance HInotify to add a new watch for the directory DirFullPath. The related information 
is stored in a new directory watch added to the Watches array. 

Note: DirFullPath must be a valid directory, not a directory symbolic link. Otherwise, the watch would be created for the link itself, 
not for the directory it points to.

- DirRelativePath: Pointer to the string containing the relative path of the directory.
- DirFullPath: Pointer to the string containing the full (absolute) path of the directory.
- WatchNodeOfDir: Optional. Pointer to the corresponding watch node of the directory, if one is created already.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).

--*/
{
    SDSTATUS    status;
    DIR_WATCH   *newDirWatch;
    __int32     newHWatch;

    // PREINIT.

    status = STATUS_FAIL;
    newDirWatch = NULL;
    newHWatch = -1;

    // Parameter validation.
    
    if (NULL == DirFullPath || 0 == DirFullPath[0])
    {
        printf("[SyncDir] Error: CreateDirWatchForDirectory(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }    
    if (NULL == DirRelativePath || 0 == DirRelativePath[0])
    {
        printf("[SyncDir] Error: CreateDirWatchForDirectory(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: CreateDirWatchForDirectory(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    
    if (gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: CreateDirWatchForDirectory(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }    
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: CreateDirWatchForDirectory(): Invalid parameter 5.\n");
        return STATUS_FAIL;
    }    


    // INIT.

    // Check for resize.

    if (gWatchesArrayCapacity == (*NumberOfWatches))
    {
        printf("[SyncDir] Info: CreateDirWatchForDirectory(): The array of DirWatch structures is being resized. \n");

        status = ResizeDirWatchArray(Watches, (*NumberOfWatches));
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateDirWatchForDirectory(): Failed to execute ResizeDirWatchArray().\n");
            status = STATUS_FAIL;
            goto cleanup_CreateDirWatchForDirectory;
        }    
    }


    //
    // Start main processing:
    //


    // Create Inotify watch.

    newHWatch = inotify_add_watch(HInotify, DirFullPath, SD_OPERATIONS_TO_WATCH);
    if (newHWatch < 0)
    {
        perror("[SyncDir] Error: CreateDirWatchForDirectory(): Could not create the new Inotify watch.\n");
        status = STATUS_FAIL;
        goto cleanup_CreateDirWatchForDirectory;
    }


    // Store Inotify watch and create a DirWatch.

    newDirWatch = &(*Watches)[*NumberOfWatches];                           // Use "newDirWatch" for readability.

    newDirWatch->HWatch = newHWatch;
    strcpy(newDirWatch->DirRelativePath, DirRelativePath);
    strcpy(newDirWatch->DirFullPath, DirFullPath);
    newDirWatch->TreeNode = WatchNodeOfDir;                             // May be NULL.
    (*NumberOfWatches) ++;


    fprintf(g_SD_STDLOG, "[SyncDir] Info: New DirWatch added (#%u): \n - Watch: [%d] \n - Relative path: [%s] \n - Full path: [%s] \n",
        (*NumberOfWatches) - 1, newDirWatch->HWatch, newDirWatch->DirRelativePath, newDirWatch->DirFullPath);



    // If here, everything worked well.
    status = STATUS_SUCCESS;


    // UNINIT. Cleanup.
    cleanup_CreateDirWatchForDirectory:

    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        // Nothing to clean for the moment.
    }

    return status;
} // CreateDirWatchForDirectory



//
// GetDirWatchIndexByHandle
//
SDSTATUS
GetDirWatchIndexByHandle(
    __in DWORD      HWatchToFind,
    __in PDIR_WATCH Watches,
    __in DWORD      NumberOfWatches,
    __out DWORD     *WatchIndexFound
    )
/*++
Description: The routine searches for a directory watch (DIR_WATCH) in a given array, that contains a given watch handle (HWatch field value).
If found, the routine outputs the index of the directory watch at the address pointed by WatchIndexFound.

- HWatchToFind: Integer representing the watch handle to be found.
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Size of the Watches array.
- WatchIndexFound: Pointer to where the routine outputs the index of the directory watch having the HWatch field equal to HWatchToFind argument.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    DWORD i;

    // PREINIT;
    i = 0;

    // Parameter validation.    
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: GetDirWatchIndexByHandle(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    
    if (gWatchesArrayCapacity < NumberOfWatches)
    {
        printf("[SyncDir] Error: GetDirWatchIndexByHandle(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    
    if (NULL == WatchIndexFound)
    {
        printf("[SyncDir] Error: GetDirWatchIndexByHandle(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }    


    // INIT.
    // --

    // Main processing:

    // Search the directory watch with the corresponding watch descriptor.
    for (i=0; i<NumberOfWatches; i++)
    {
        if ((unsigned)Watches[i].HWatch == HWatchToFind)
        {
            (*WatchIndexFound) = i;
            break;
        }
    }

    return STATUS_SUCCESS;
} // GetDirWatchIndexByHandle()



//
// InitDirWatchArray
//
SDSTATUS
InitDirWatchArray(
    __inout DIR_WATCH   *Watches,
    __in    DWORD       ArrayCapacity
    )
/*++
    Description: The function initializes with neutral (default) values an array of directory watches.

    - Watches: Pointer to the array of directory watches.
    - ArrayCapacity: Length up to which the routine initializes the Watches array (in number of array elements).

    Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    DWORD i;

    // PREINIT.
    i = 0;

    // Parameter validation.
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: InitDirWatchArray(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if ((ArrayCapacity <= 0) || (gWatchesArrayCapacity < ArrayCapacity))
    {
        printf("[SyncDir] Error: InitDirWatchArray(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }

    // INIT.
    // --

    // Main processing.

    // Set a default value for the contents:
    for (i=0; i<ArrayCapacity; i++)
    {
        Watches[i].HWatch = -1;
        Watches[i].DirFullPath[0] = 0;
        Watches[i].DirRelativePath[0] = 0;
        Watches[i].TreeNode = NULL;
    }

    return STATUS_SUCCESS;
} // InitDirWatchArray()



//
// ResizeDirWatchArray
//
SDSTATUS
ResizeDirWatchArray(
    __inout DIR_WATCH   **Watches,
    __in    DWORD       NumberOfWatches
    )
/*++
Description: The function allocates more space for the array pointed by (*Watches), increasing its current capacity (gWatchesArrayCapacity) 
by adding SD_INITIAL_NR_OF_WATCHES elements.

- Watches: Address of the pointer to the array of DIR_WATCH to be resized.
- NumberOfWatches: The number of DIR_WATCH elements in the (*Watches) array.

Return value: STATUS_SUCCESS on success. STATUS_FAIL otherwise.
--*/
{
    SDSTATUS    status;
    DIR_WATCH   *resizedWatches;
    DIR_WATCH   *oldWatches;
    DWORD       i;    
    
    // PREINIT.
    
    status = STATUS_FAIL;
    resizedWatches = NULL;
    oldWatches = NULL;

    // Parameter validation.
    
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: ResizeDirWatchArray(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (gWatchesArrayCapacity < NumberOfWatches)
    {
        printf("[SyncDir] Error: ResizeDirWatchArray(): Invalid parameter 2.\n");
        return STATUS_FAIL;        
    }


    // INIT.
    // --


    // Main processing.


    // Allocate more space for the array.

    resizedWatches = (DIR_WATCH*) malloc((gWatchesArrayCapacity + SD_INITIAL_NR_OF_WATCHES) * sizeof(DIR_WATCH));
    if (NULL == resizedWatches)
    {
        perror("[SyncDir] Error: ResizeDirWatchArray(): Failed to allocate memory.\n");
        status = STATUS_FAIL;
        goto cleanup_ResizeDirWatchArray;
    }


    // Update global array capacity.

    gWatchesArrayCapacity = gWatchesArrayCapacity + SD_INITIAL_NR_OF_WATCHES;


    // Initialize the resized array.

    status = InitDirWatchArray(resizedWatches, gWatchesArrayCapacity);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: ResizeDirWatchArray(): Failed at InitDirWatchArray().\n");
        status = STATUS_FAIL;
        goto cleanup_ResizeDirWatchArray;
    }


    // Copy elements to the resized array

    printf("1 \n");
    for (i=0; i<NumberOfWatches; i++)
    {
        resizedWatches[i].HWatch = (*Watches)[i].HWatch;
        strcpy(resizedWatches[i].DirFullPath, (*Watches)[i].DirFullPath);
        strcpy(resizedWatches[i].DirRelativePath, (*Watches)[i].DirRelativePath);
        resizedWatches[i].TreeNode = (*Watches)[i].TreeNode;
    }        
    printf("2 \n");



    // If here, everything ok.    
    status = STATUS_SUCCESS;

    // Save the old array for deletion.
    oldWatches = (*Watches);
    // Output should point to the new array.
    (*Watches) = resizedWatches;


    // UNINIT. Cleanup.
    cleanup_ResizeDirWatchArray:

    if (SUCCESS(status))
    {
        if (NULL != oldWatches)            // Destroy old array.
        {
            free(oldWatches);
            oldWatches = NULL;
        }
        printf("watches[0]=[%d], watches[n]=[%d], path[0]=[%s], path[n]=[%s] \n", 
            (*Watches)[0].HWatch,
            (*Watches)[NumberOfWatches-1].HWatch,
            (*Watches)[0].DirRelativePath,
            (*Watches)[NumberOfWatches-1].DirRelativePath
            );
    }
    else                        
    {
        if (NULL != (*Watches))
        {
            free((*Watches));
            (*Watches) = NULL;
        }
        if (NULL != resizedWatches)     // If fail, destroy both arrays.
        {
            free(resizedWatches);
            resizedWatches = NULL;
        }
    }

    return status;
} // ResizeDirWatchArray()



//
// CreateWatchStructuresForAllSubdirectories
//
SDSTATUS
CreateWatchStructuresForAllSubdirectories(
    __in    DWORD       RootDirWatchIndex, 
    __inout DIR_WATCH   **Watches, 
    __inout DWORD       *NumberOfWatches,
    __in    __int32     HInotify    
    )
/*++
Description: The routine recursively creates all the watch structures for a given directory subtree, necessary for integration
in the SyncDir monitoring process. The watch structures include Inotify watches, directory watches (DIR_WATCH) and watch nodes
(DIR_WATCH_NODE). The directory subtree is represented by RootDirWatchIndex argument.

Note: Process not including the first directory! The routine does not create data structures for the root directory (RootDirWatchIndex),
but only for its contents. The routine assumes the root's structures already exist.

Note: RootDirWatchIndex does not denote the system's root ("/"), nor the main directory of SyncDir monitoring.

- RootDirWatchIndex: Index, in the Watches array, of the WatchDir where the directory subtree starts.
- Watches: Pointer to the address of the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Handle of the Inotify instance containing the Inotify watches (which monitor the directories).

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).

--*/
{
    SDSTATUS        status;
    DIR             *dirStream;
    struct dirent   *subfile;
    struct stat     subfileStat;
    char            subdirFullPath[SD_MAX_PATH_LENGTH];
    char            subdirRelativePath[SD_MAX_PATH_LENGTH];    
    DWORD           crtWatchIndex;    

    // PREINIT.

    status = STATUS_FAIL;
    dirStream = NULL;
    subfile = NULL;
    subdirFullPath[0] = 0;
    subdirRelativePath[0] = 0;
    crtWatchIndex = 0;
    
    // Parameter validation.

    if (HInotify < 0)
    {
        printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches || NULL == (*Watches))
    {
        printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }
    if (NULL == NumberOfWatches || gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }   
    if ((*NumberOfWatches) < RootDirWatchIndex)
    {
        printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }



    // INIT (start).

    // Initialize the directory stream for recursive subdirectories iteration.

    dirStream = opendir((*Watches)[RootDirWatchIndex].DirFullPath);
    if (NULL == dirStream)
    {
        perror("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Could not execute opendir() for the parent directory.\n");
        status = STATUS_WARNING;
        goto cleanup_CreateWatchStructuresForAllSubdirectories;
        // File may not exist anymore, or the user renamed/moved the file meanwhile.
    }

    // --> INIT (end).



    //
    // Start main processing:
    //


    //
    // LOOP THE WATCH STRUCTURES CREATION.
    //

    // Iterate the subfiles of the parent directory (== dirStream).
    // Find the directory files and create all the watch structures for them.

    while (1)
    {


        // Get next file.

        subfile = readdir(dirStream);
        if (NULL == subfile)
        {
            // end of directory stream
            break;
        }    
        if (0 == strcmp(".", subfile->d_name) || 0 == strcmp("..", subfile->d_name))        // Avoid containing dir and parent dir.
        {
            continue;
        }

        // Get file status: fstatat() with AT_SYMLINK_NOFOLLOW, to not resolve symbolic links -> This behaves like lstat().
        // -> Because we need information about the file itself, not about the file it is linking to.
        
        if (fstatat(dirfd(dirStream), subfile->d_name, &subfileStat, AT_SYMLINK_NOFOLLOW) < 0)
        {
            perror("[SyncDir] Warning: CreateWatchStructuresForAllSubdirectories(): Could not execute fstatat() for the file.\n");
                                                                    // Possibly the file was (re)moved meanwhile. (Or altered.)            
            continue;
        }



        // If sym link ==> log & ignore (continue). 
        // -> Because the link can point to a file outside of the Main Directory.
        // -> If link points inside the Main Directory, the real path will anyway be watched.

        if (S_ISLNK(subfileStat.st_mode))
        {
            printf("[SyncDir] Info: Symbolic link found [%s]. Ignoring.\n", subfile->d_name);
            continue;
        }



        // If directory:
        // 1. Create Inotify watch and directory watch.
        // 2. Create watch node.
        // 3. Recursively treat directory content / subdirectories.

        if (S_ISDIR(subfileStat.st_mode))
        {


            // Get index of DirWatch element to create.

            crtWatchIndex = (*NumberOfWatches);

            // Get directory's full and relative paths.

            snprintf(subdirFullPath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[RootDirWatchIndex].DirFullPath, subfile->d_name);
            snprintf(subdirRelativePath, SD_MAX_PATH_LENGTH, "%s/%s", (*Watches)[RootDirWatchIndex].DirRelativePath, subfile->d_name);

            fprintf(g_SD_STDLOG, "[SyncDir] Info: Adding watch structures for: \n - subdir full path [%s] \n - subdir relative path [%s].\n", 
                subdirFullPath, subdirRelativePath);



            //
            // 1. CREATE NEW INOTIFY WATCH AND DIRECTORY WATCH (for the subdirectory).
            //

            status = CreateDirWatchForDirectory(subdirRelativePath, subdirFullPath, NULL, Watches, NumberOfWatches, HInotify);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Failed at CreateDirWatchForDirectory().\n");
                printf("==> Error for subdir [%s] and parent dir [%s].\n", subdirFullPath, (*Watches)[RootDirWatchIndex].DirFullPath);
                status = STATUS_FAIL;
                goto cleanup_CreateWatchStructuresForAllSubdirectories;
            };
            fprintf(g_SD_STDLOG, "[SyncDir] Info: New DirWatch[%d]=[%d] added for a subdirectory [%s].\n", crtWatchIndex, 
                (*Watches)[crtWatchIndex].HWatch, (*Watches)[crtWatchIndex].DirRelativePath);



            //
            // 2. CREATE NEW WATCH NODE (for the subdirectory).
            //


            status = CreateWatchNode(crtWatchIndex, (*Watches));
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Failed at CreateWatchNode().\n");
                status = STATUS_FAIL;
                goto cleanup_CreateWatchStructuresForAllSubdirectories;
            }
            fprintf(g_SD_STDLOG, "[SyncDir] Info: New WatchNode created for the subdirectory. \n");


            status = AddChildWatchNodeToTree(FALSE, crtWatchIndex, RootDirWatchIndex, subfile->d_name, (*Watches));
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: CreateWatchStructuresForAllSubdirectories(): Failed at AddChildWatchNodeToTree().\n");
                status = STATUS_FAIL;
                goto cleanup_CreateWatchStructuresForAllSubdirectories;
            }
            fprintf(g_SD_STDLOG, "[SyncDir] Info: The new WatchNode was added to the watch tree. \n");



            // 
            // 3. RECURSIVELY TREAT SUBDIRECTORIES.
            // 

            // Explore the current subdirectory. Recursively add watches for all inner subdirectories.

            status = CreateWatchStructuresForAllSubdirectories(crtWatchIndex, Watches, NumberOfWatches, HInotify);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Warning: CreateWatchStructuresForAllSubdirectories() failed for a subfile. \
                    Continuing execution... \n");   // Subdirectory may have been removed meanwhile (?).
                continue;
            }

        } //--> if (S_ISDIR())


    } //--> while(1)


            
    // If here, everything went ok.
    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    cleanup_CreateWatchStructuresForAllSubdirectories:

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
} // CreateWatchStructuresForAllSubdirectories()



//
// CltMonitorPartition
//
SDSTATUS
CltMonitorPartition(
    __in char       *MainDirPath,
    __in __int32    CltSock
    )
/*++
Description: Main SyncDir client routine, where the monitoring begins. The routine creates the initial SyncDir data structures and starts the
monitoring of the main directory (MainDirPath). The routine then launches procedures for monitoring the whole directory tree of MainDirPath, 
processing the events and updating the SyncDir server. This maintains the synchronization of content between client and server partitions.

- MainDirPath: Pointer to the string containing the path towards the directory tree that SyncDir will monitor. This can be a file system partition.
- CltSock: Descriptor representing the socket connection with the SyncDir server application.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).

--*/
{
    SDSTATUS    status;
    __int32     hInotify;
    __int32     hMainDirWatch;
    DWORD       numberOfWatches;
    BOOL        isDirValid;
    DIR_WATCH   *watches;                   // Array of real-time monitors -> capturing any subdirectory file change.

    // PREINIT.

    status = STATUS_FAIL;
    hInotify = -1;
    hMainDirWatch = -1;
    numberOfWatches = 0;
    isDirValid = FALSE;
    watches = NULL;             

    // Parameter validation.

    if (NULL == MainDirPath)
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    status = IsDirectoryValid(MainDirPath, &isDirValid);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed at executing IsDirectoryValid().\n");
        return STATUS_FAIL;
    }
    if (FALSE == isDirValid)
    {
        printf("[SyncDir] Error: CltMonitorPartition(): The provided directory path is not valid.\n");
        return STATUS_FAIL;
    }



    // INIT (start).


    // Allocate space and initialize the watch array == the array of directory watches (DIR_WATCH structures).
    // The watch array will store the essential information for monitoring all the directories on the client partition.

    watches = (DIR_WATCH*) malloc(SD_INITIAL_NR_OF_WATCHES * sizeof(DIR_WATCH));
    if (NULL == watches)
    {
        perror("[SyncDir] Error: CltMonitorPartition(): Failed to allocate memory for the watches array.\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }

    status = InitDirWatchArray(watches, SD_INITIAL_NR_OF_WATCHES);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed to execute InitDirWatchArray().\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }    

    numberOfWatches = 0;


    // Create an Inotify instance.

    hInotify = inotify_init();
    if (hInotify < 0)
    {
        perror("[SyncDir] Error: CltMonitorPartition(): could not create an Inotify instance. \n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }


    // --> INIT (end).



    //
    // Start main processing:
    //


    // 1. CREATE INOTIFY WATCH FOR THE MAIN DIRECTORY

    // Obtain an Inotify watch (descriptor/handle) for the main directory ("root dir").
    // Monitoring begins here.

    hMainDirWatch = inotify_add_watch(hInotify, MainDirPath, SD_OPERATIONS_TO_WATCH);
    if (hMainDirWatch < 0)
    {
        perror("[SyncDir] Error: CltMonitorPartition(): could not add Inotify watch for the Main Directory. \n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }
    fprintf(g_SD_STDLOG, "[SyncDir] Info: Added DirWatch[0]=[%d]\n", hMainDirWatch);



    // 2. CREATE DIRECTORY WATCH FOR THE MAIN DIRECTORY.

    // Store the first Inotify watch and related information in the "root directory watch" (watches[0]).
    
    watches[0].HWatch = hMainDirWatch;

    if (NULL == realpath(MainDirPath, watches[0].DirFullPath))                          // realpath() stores the path to watches[0].
    {
        perror("[SyncDir] Error: CltMonitorPartition(): could not get the real path of the Main Directory.\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }

    strcpy(watches[0].DirRelativePath, ".");                                            // Main directory name (".").
    watches[0].TreeNode = NULL;

    numberOfWatches = 1;



    // 3. CREATE WATCH NODE FOR THE MAIN DIRECTORY
    
    // Create the first watch node (DIR_WATCH_NODE), i.e. main directory's node. Set it as root in the watch tree (DIR_WATCH.TreeNode field).
    // At creation, each watch node has Depth equal to 0.
    // Its Depth value changes when it is inserted in the watch tree, relatively to the Depth value of its parent node.

    status = CreateWatchNode(0, watches);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed to execute CreateWatchNode().\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }    
    status = AddChildWatchNodeToTree(TRUE, 0, -1, ".", watches);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed to execute AddChildWatchNodeToTree().\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }    



    // 4. WATCH WHOLE SUBDIRECTORY TREE STRUCTURE

    // - Inotify watches for all subdirectories.
    // - watch nodes for all subdirectories.
    // - directory watches for all subdirectories.
    // The watch array and the watch tree are populated here, recursively.

    status = CreateWatchStructuresForAllSubdirectories(0, &watches, &numberOfWatches, hInotify);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed to execute CreateWatchStructuresForAllSubdirectories().\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }    



    // 5. PROCESS EVENTS

    // Wait for events from all subdirectory watches. 
    // Here, the SyncDir client informs the SyncDir server of all changes. 

    status = WaitForEventsAndProcessChanges(MainDirPath, &watches, &numberOfWatches, hInotify, CltSock);
    if (!(SUCCESS(status)))
    {
        printf("[SyncDir] Error: CltMonitorPartition(): Failed to execute WaitForEventsAndProcessChanges().\n");
        status = STATUS_FAIL;
        goto cleanup_CltMonitorPartition;
    }    



    // If here, everything went well.
    status = STATUS_SUCCESS;

    // UNINIT. Cleanup.
    cleanup_CltMonitorPartition:

    if (SUCCESS(status))
    {
        if (NULL != watches)
        {
            if (-1 != hInotify)
            {
                DWORD i;
                for (i=0; i<numberOfWatches; i++)
                {
                    if (-1 != watches[i].HWatch)
                    {
                        inotify_rm_watch(hInotify, watches[i].HWatch);
                    }
                    if (NULL != watches[i].TreeNode)
                    {
                        FreeAndNullWatchNode(&watches[i].TreeNode);
                    }
                }
                close(hInotify);
                hInotify = -1;
            }

            free(watches);
            watches = NULL;
        }
    }
    else
    {
        if (NULL != watches)
        {
            if (-1 != hInotify)
            {
                DWORD i;
                for (i=0; i<numberOfWatches; i++)
                {
                    if (-1 != watches[i].HWatch)
                    {
                        inotify_rm_watch(hInotify, watches[i].HWatch);
                    }
                    if (NULL != watches[i].TreeNode)
                    {
                        FreeAndNullWatchNode(&watches[i].TreeNode);
                    }
                }
                close(hInotify);
                hInotify = -1;
            }

            free(watches);
            watches = NULL;
        }
    }

    return status;
} // CltMonitorPartition().



