
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_clt_watch_tree.h"


//
// GetChildWatchNodeByDirName
//
SDSTATUS
GetChildWatchNodeByDirName(
    __in PDIR_WATCH_NODE    ParentNode,
    __in char               *DirName,
    __out PDIR_WATCH_NODE   *ChildNode
    )
/*++
Description: The routine searches among the direct sub-nodes of ParentNode for the node with the same name (the DirName field in the
DIR_WATCH_NODE structure) as the one pointed by DirName. 
If a node is found, its address is stored at ChildNode, otherwise, NULL is output.

- ParentNode: The address of the node whose child is to be found.
- DirName: The address of the string containing the name to be found.
- ChildNode: The address where the routine outputs the node address, if one is found, or NULL, otherwise.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    DWORD i;
    DIR_WATCH_NODE *ChildFound;

    // PREINIT.

    status = STATUS_FAIL;
    i = 0;
    ChildFound = NULL;

    // Parameter validation.

    if (NULL == ParentNode)
    {
        printf("[SyncDir] Error: GetChildWatchNodeByDirName(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == DirName || 0 == DirName[0])
    {
        printf("[SyncDir] Error: GetChildWatchNodeByDirName(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == ChildNode)
    {
        printf("[SyncDir] Error: GetChildWatchNodeByDirName(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Search for the node with the directory name equal to DirName.

        for (i=0; i<ParentNode->Subdirs.size(); i++)
        {
            if (0 == strcmp(DirName, ParentNode->Subdirs[i]->DirName.c_str()))
            {
                ChildFound = ParentNode->Subdirs[i];
                break;
            }
        }

        if (NULL == ChildFound)
        {
            //printf("[SyncDir] Info: GetChildWatchNodeByDirName(): Child node [%s] not found. \n", DirName);
        }


        // If here, everything is ok.
        // Output the result.
        
        (*ChildNode) = ChildFound;

        status = STATUS_SUCCESS;

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: GetChildWatchNodeByDirName(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: GetChildWatchNodeByDirName(): Unkown exception.\n");
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
} // GetChildWatchNodeByDirName()



//
// FreeAndNullWatchNode
//
SDSTATUS
FreeAndNullWatchNode(
    __inout DIR_WATCH_NODE **WatchNode
    )
/*++
Description: The routine destroys the watch node WatchNode.

- DirWatchNode: Pointer to the address of the watch node destroyed by the routine.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.
    status = STATUS_FAIL;

    // Parameter validation.
    if (NULL == WatchNode)
    {
        printf("[SyncDir] Error: FreeAndNullWatchNode: Invalid parameter 1. \n");
        return STATUS_FAIL;
    }


    __try
    {

        if (NULL != (*WatchNode))
        {
            delete ((*WatchNode));
            *WatchNode = NULL;
        }

        status = STATUS_SUCCESS;


    } // --> __try
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: DeleteWatchAndNodeOfDir(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    return status;        
} // FreeAndNullWatchNode.



//
// DeleteWatchAndNodeOfDir
//
SDSTATUS
DeleteWatchAndNodeOfDir(
    __inout DIR_WATCH_NODE  *DirWatchNode,
    __inout DIR_WATCH       *Watches,
    __inout DWORD           *NumberOfWatches,
    __in __int32            HInotify    
    )
/*++
Description: The routine deletes and destroys the watch node DirWatchNode, its corresponding DIR_WATCH structure (from the Watches array) 
and removes its directory watch from the HInotify instance.

- DirWatchNode: Pointer to the watch node whose watch-related structures are destroyed by the routine.
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance associated to all the watches in the Watches array.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.
    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == DirWatchNode)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Invalid parameter 1.\n");
        return STATUS_FAIL;        
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Invalid parameter 2.\n");
        return STATUS_FAIL;        
    }
    if (NULL == NumberOfWatches)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Invalid parameter 3.\n");
        return STATUS_FAIL;        
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Invalid parameter 4.\n");
        return STATUS_FAIL;        
    }

    __try
    {
        // INIT.
        // --


        // Main processing:


        // Destroy and delete the DIR_WATCH structure associated to the node DirWatchNode.

        status = DeleteDirWatchByIndex(DirWatchNode->DirWatchIndex, Watches, NumberOfWatches, HInotify );
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Failed to execute DeleteDirWatchByIndex().\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Delete DirWatchNode from its parent node.

        auto nodePos = std::find(DirWatchNode->Parent->Subdirs.begin(), DirWatchNode->Parent->Subdirs.end(), DirWatchNode);
        if (DirWatchNode->Parent->Subdirs.end() != nodePos)
        {
            DirWatchNode->Parent->Subdirs.erase(nodePos);  
        }
        else
        {
            printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Failed to erase DIR_WATCH_NODE structure from the tree.\n " );
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Destroy the DirWatchNode structure.

        free(DirWatchNode);
        DirWatchNode = NULL;        


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
        cout << "[SyncDir] Error: DeleteWatchAndNodeOfDir(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: DeleteWatchAndNodeOfDir(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        if (NULL != DirWatchNode)
        {            
            free(DirWatchNode);
            DirWatchNode = NULL;                    
        }
    }

    return status;
} // DeleteWatchAndNodeOfDir()



//
// DeleteWatchesAndNodesOfSubdirs
//
SDSTATUS
DeleteWatchesAndNodesOfSubdirs(
    __inout DIR_WATCH_NODE  *StartNode,
    __inout DIR_WATCH       *Watches,
    __inout DWORD           *NumberOfWatches,
    __in __int32            HInotify        
    )
/*++
Description: The routine deletes and destroys all the watch-related structures of the subtree starting at the watch node StartNode.
This includes the DIR_WATCH_NODE structures, the DIR_WATCH structures and removal of the associated Inotify watches for all subdirectories.

- StartNode: Pointer to the watch node whose subtree is deleted and destroyed by the routine (the node itself included).
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance associated to all the watches in the Watches array.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS                        status;
    std::stack<PDIR_WATCH_NODE>     stack;
    PDIR_WATCH_NODE                 nodeToExplore;

    // PREINIT.

    status = STATUS_FAIL;
    nodeToExplore = NULL;

    // Parameter validation.

    if (NULL == StartNode)
    {
        printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Invalid parameter 2.\n");
        return STATUS_FAIL;        
    }
    if (NULL == NumberOfWatches)
    {
        printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Invalid parameter 3.\n");
        return STATUS_FAIL;        
    }
    if (HInotify < 0)
    {
        printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Invalid parameter 4.\n");
        return STATUS_FAIL;        
    }


    __try
    {
        // INIT.
        // --


        // Main processing:

        // Traverse the tree and perform node destruction/deletion in postorder.
        // Use a stack to efficiently perform the postorder processing.

        // Prepare to destroy the subtree (including StartNode).
        stack.push(StartNode);

        // Destroy subtree of StartNode.
        while (FALSE == stack.empty())
        {
            nodeToExplore = stack.top(); 
            stack.pop();
            if (0 == nodeToExplore->Subdirs.size())
            {
                status = DeleteWatchAndNodeOfDir(nodeToExplore, Watches, NumberOfWatches, HInotify);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Failed to execute DeleteWatchAndNodeOfDir(). \n");
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }
            }
            else
            {
                DWORD i;

                stack.push(nodeToExplore);
                for (i=0; i<nodeToExplore->Subdirs.size(); i++)
                {
                    stack.push(nodeToExplore->Subdirs[i]);
                }                
            }
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
        cout << "[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: DeleteWatchesAndNodesOfSubdirs(): Unkown exception.\n");
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
} // DeleteWatchesAndNodesOfSubdirs()



//
// InitWatchNode
//
SDSTATUS
InitWatchNode(
    __out DIR_WATCH_NODE *WatchNode
    )
/*++
Description: The routine initializes a watch node with default values.

- WatchNode: Pointer to the watch node to be initialized.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == WatchNode)
    {
        printf("[SyncDir] Error: InitWatchNode(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }

    __try
    {
        // INIT.
        // --

        // Main processing:

        WatchNode->DirWatchIndex = 0;
        WatchNode->Parent = NULL;
        WatchNode->Depth = 0;
        WatchNode->DirName = "<NO_EXISTENT_DIR>";

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
        cout << "[SyncDir] Error: InitWatchNode(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: InitWatchNode(): Unkown exception.\n");
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
} // InitWatchNode()



//
// AddChildWatchNodeToTree
//
SDSTATUS
AddChildWatchNodeToTree(
    __in BOOL           IsRootNode,
    __in DWORD          ChildWatchIndex,
    __in DWORD          ParentWatchIndex,
    __in char           *DirName,
    __inout DIR_WATCH   *Watches
    )
/*++
Description: The routine adds an already built watch node in the tree of watches.

- IsRootNode: Indicates if the node is the first node of the tree (root), or not.
- ChildWatchIndex: Index in the Watches array, corresponding to the watch node that the routine adds to the tree.
- ParentWatchIndex: Index in the Watches array, corresponding to the parent watch node.
- DirName: Pointer to the string containing the directory name of the new watch node.    
- Watches: Pointer to the array of directory watches.

Return value: STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (FALSE == IsRootNode)
    {
        if (gWatchesArrayCapacity < ChildWatchIndex)
        {
            printf("[SyncDir] Error: AddChildWatchNodeToTree(): Invalid parameter 2. \n");
            return STATUS_FAIL;
        }
        if (gWatchesArrayCapacity < ParentWatchIndex)
        {
            printf("[SyncDir] Error: AddChildWatchNodeToTree(): Invalid parameter 3. \n");
            return STATUS_FAIL;
        }      
    }
    if (NULL == DirName || 0 == DirName[0])
    {
        printf("[SyncDir] Error: AddChildWatchNodeToTree(): Invalid parameter 4. \n");
        return STATUS_FAIL;
    }        
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: AddChildWatchNodeToTree(): Invalid parameter 5. \n");
        return STATUS_FAIL;
    }

    __try
    {
        // INIT.
        // --

        // Main processing:

        // If node is root, keep the presets (NULL parent, 0 depth).

        if (FALSE == IsRootNode)
        {
            Watches[ParentWatchIndex].TreeNode->Subdirs.push_back(Watches[ChildWatchIndex].TreeNode);       // Add the node to the child list.
            
            Watches[ChildWatchIndex].TreeNode->Parent = Watches[ParentWatchIndex].TreeNode;                 // Set the child's parent node.

            Watches[ChildWatchIndex].TreeNode->Depth = Watches[ParentWatchIndex].TreeNode->Depth + 1;       // Set the child's depth.
        }
        if (TRUE == IsRootNode)
        {
            Watches[ChildWatchIndex].TreeNode->Parent = NULL;
            Watches[ChildWatchIndex].TreeNode->Depth = 0;

        }

        // Set current info (index, dir name).

        Watches[ChildWatchIndex].TreeNode->DirWatchIndex = ChildWatchIndex;
        Watches[ChildWatchIndex].TreeNode->DirName.assign( DirName );



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
        cout << "[SyncDir] Error: AddChildWatchNodeToTree(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: AddChildWatchNodeToTree(): Unkown exception.\n");
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
} // AddChildWatchNodeToTree()



//
// CreateWatchNode
//
SDSTATUS
CreateWatchNode(
    __in DWORD          CrtWatchIndex,
    __inout DIR_WATCH   *Watches
    )
/*++
Description: The routine creates the watch node for a given directory watch. The new watch node is associated to the DIR_WATCH structure
identified by CrtWatchIndex.

- CrtWatchIndex: Index in the Watches array, corresponding to the new watch node that the routine creates.
- Watches: Pointer to the array of directory watches.

Return value: STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (gWatchesArrayCapacity < CrtWatchIndex)
    {
        printf("[SyncDir] Error: CreateWatchNode(): Invalid parameter 1. \n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: CreateWatchNode(): Invalid parameter 3. \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:


        // Allocate watch node space:

        Watches[CrtWatchIndex].TreeNode = new DIR_WATCH_NODE();
        if (NULL == Watches[CrtWatchIndex].TreeNode)
        {
            perror("[SyncDir] Error: CreateWatchNode(): Failed to allocate space - new-expression. \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        fprintf(g_SD_STDLOG, "[SyncDir] Info: New WatchNode space allocated successfully (size %lu). \n", sizeof(DIR_WATCH_NODE));



        // Set the initial/default values (of the watch node):

        status = InitWatchNode(Watches[CrtWatchIndex].TreeNode); // Important. Due to its presets (e.g. Depth = 0, the main directory's Depth).
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CreateWatchNode(): Failed to execute InitWatchNode(). \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }
        fprintf(g_SD_STDLOG, "[SyncDir] Info: New WatchNode initialized successfully. \n");


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
        cout << "[SyncDir] Error: CreateWatchNode(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: CreateWatchNode(): Unkown exception.\n");
        status = STATUS_FAIL;
    }


    // UNINIT. Cleanup.
    if (SUCCESS(status))
    {
        // Nothing to clean for the moment.
    }
    else
    {
        if (NULL != Watches[CrtWatchIndex].TreeNode)
        {
            delete (Watches[CrtWatchIndex].TreeNode);
            Watches[CrtWatchIndex].TreeNode = NULL;
        }
    }

    return status;
} // CreateWatchNode()



//
// UpdatePathsForSubTreeWatches
//
SDSTATUS
UpdatePathsForSubTreeWatches(
    __inout DIR_WATCH_NODE      *StartNode,
    __inout DIR_WATCH           *Watches
    )
/*++
Description: The routine updates the full paths and relative paths of all directory watches of a watch tree, plus the depths of all
watch nodes in the tree. 

- StartNode: Pointer to the watch node where the watch tree starts.
- Watches: Pointer to the array of directory watches.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS                        status;
    DIR_WATCH_NODE                  *nodeToExplore;
    std::queue<DIR_WATCH_NODE*>     queue;

    // PREINIT.

    status = STATUS_FAIL;
    nodeToExplore = NULL;

    // Parameter validation.

    if (NULL == StartNode)
    {
        printf("[SyncDir] Error: UpdatePathsForSubTreeWatches(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: UpdatePathsForSubTreeWatches(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }

    __try
    {
        // INIT.
        // --

        // Main procecssing:

        // Use queue because we need to update the paths progressively (hence we use BFS exploration).
        // Add each node and its children to the queue.
        // Meanwhile, pop nodes one by one and update paths and depths.

        queue.push(StartNode);

        while (FALSE == queue.empty())
        {
            DWORD i;

            nodeToExplore = queue.front(); 
            queue.pop();

            sprintf(Watches[nodeToExplore->DirWatchIndex].DirRelativePath, "%s/%s", Watches[nodeToExplore->Parent->DirWatchIndex].DirRelativePath,
                nodeToExplore->DirName.c_str());
            sprintf(Watches[nodeToExplore->DirWatchIndex].DirFullPath, "%s/%s", Watches[nodeToExplore->Parent->DirWatchIndex].DirFullPath,
                nodeToExplore->DirName.c_str());
            nodeToExplore->Depth = nodeToExplore->Parent->Depth + 1;

            for (i=0; i<nodeToExplore->Subdirs.size(); i++)
            {
                queue.push(nodeToExplore->Subdirs[i]);
            }                
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
        cout << "[SyncDir] Error: UpdatePathsForSubTreeWatches(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: UpdatePathsForSubTreeWatches(): Unkown exception.\n");
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
} // UpdatePathsForSubTreeWatches()



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
    )
/*++
Description: The routine checks if there exists a child watch node for WatchNodeOfParentDir with the same name as DirName. If so,
the routine destroys and deletes the node, along with its all watch-related structures (the DIR_WATCH inside Watches, the Inotify watch
enabled upon the Inotify instance HInotify).

- DirName: Pointer to the string containing the directory name of the node to be found.
- WatchNodeOfParentDir: Pointer to the parent node of the watch node to be found.
- Watches: Pointer to the array of directory watches.
- NumberOfWatches: Pointer to the size of the Watches array.
- HInotify: Descriptor of the Inotify instance containing the Inotify watches of the monitored directories.

Return value: STATUS_SUCCESS upon success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS status;
    DIR_WATCH_NODE *watchNodeWithSameName;

    // PREINIT.

    status = STATUS_FAIL;
    watchNodeWithSameName = NULL;

    // Parameter validation.

    if (NULL == DirName || 0 == DirName[0])
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Invalid paramter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == WatchNodeOfParentDir)
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Invalid paramter 2.\n");
        return STATUS_FAIL;
    }
    if (NULL == Watches)
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Invalid paratmer 3.\n");
        return STATUS_FAIL;
    }
    if (NULL == NumberOfWatches || gWatchesArrayCapacity < (*NumberOfWatches))
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Invalid paramter 4.\n");
        return STATUS_FAIL;
    }
    if ( HInotify < 0 )
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Invalid paratmer 5.\n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        
        watchNodeWithSameName = NULL;        


        // Main processing:

        // Check subnode existence (search by DirName).
        // If it exists, destroy and delete the watch node, directory watch structure and Inotify watch.

        status = GetChildWatchNodeByDirName(WatchNodeOfParentDir, DirName, &watchNodeWithSameName);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Failed to execute GetChildWatchNodeByDirName().\n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        if (NULL != watchNodeWithSameName) 
        {
            status = DeleteWatchAndNodeOfDir(watchNodeWithSameName, Watches, NumberOfWatches, HInotify);
            if (!(SUCCESS(status)))
            {
                printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Failed to execute DeleteWatchAndNodeOfDir().\n");
                status = STATUS_FAIL;
                throw SyncDirException();
            }
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
        cout << "[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: CheckWatchNodeExistenceForCleanup(): Unkown exception.\n");
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
} // CheckWatchNodeExistenceForCleanup()



