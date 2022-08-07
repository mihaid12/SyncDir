
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "syncdir_srv_hash_info_proc.h"


//
// UpdateOrDeleteHashInfosForDirPath
//
SDSTATUS
UpdateOrDeleteHashInfosForDirPath(
    __in char       *DirFullPath,
    __in char       *DirRelativePath,
    __in_opt char   *NewDirRelativePath,
    __in const char *UpdateOrDelete,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    )
/*++
Description: The routine updates or deletes all the HASH_INFO structures of the files inside a given directory and all of its
subdirectories (recursively). The directory is indicated by the DirFullPath, DirRelativePath and NewDirRelativePath arguments. The latter
is used only if an update is indicated by the UpdateOrDelete argument. In this case the file paths inside the HashInfo's are updated
to include the new path NewDirRelativePath. The behaviour of the routine (update/delete) is dictated by the UpdateOrDelete argument.

- DirFullPath: Pointer to the directory full path.
- DirRelativePath: Pointer to the directory relative path (the path is relative to the main directory of the application).
- NewDirRelativePath: Pointer to the new relative path of the directory (used in case UpdateOrDelete is set to "UPDATE").
    This argument can be NULL, if UpdateOrDelete is set to "DELETE", since it is not used.
- UpdateOrDelete: Pointer to a string indicating the routine behaviour. "UPDATE" and "DELETE" are the possible values.
- HahsInfoHMap: Reference to the map containing all the HASH_INFO structures of all the files on the server.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    DIR             *dirStream;
    struct dirent   *file;
    struct stat     fileStat;
    char            fileRelativePath[SD_MAX_PATH_LENGTH];
    char            newFileRelativePath[SD_MAX_PATH_LENGTH];
    HASH_INFO       *oldHashInfo;
    std::string     auxString;

    // PREINIT.

    status = STATUS_FAIL;
    dirStream = NULL;
    file = NULL;
    fileRelativePath[0] = 0;
    oldHashInfo = NULL;

    // Parameter validation.

    if (NULL == DirFullPath || 0 == DirFullPath[0])
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == DirRelativePath || 0 == DirRelativePath[0])
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }    
    if (0 != strcmp("UPDATE", UpdateOrDelete) && 0 != strcmp("DELETE", UpdateOrDelete))
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter 4.\n");
        return STATUS_FAIL;
    }    
    if (NULL == NewDirRelativePath && 0 == strcmp("UPDATE", UpdateOrDelete))
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter relation 3-4.\n");
        return STATUS_FAIL;
    }
    if (NULL != NewDirRelativePath && 0 == NewDirRelativePath[0])
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter 3.\n");
        return STATUS_FAIL;
    }    
    if (0 == HashInfoHMap.size())
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Invalid parameter 5.\n");
        return STATUS_FAIL;        
    }      


    __try
    {
        // INIT.
        // --

        // For maintaining clarity of the code.
        // Replace NULL with string '\0' to avoid "if" conditionals when processing paths for both "UPDATE" and "DELETE".

        if (0 == strcmp("DELETE", UpdateOrDelete))                         // UpdateOrDelete == "DELETE".
        {
            NewDirRelativePath = (char*) malloc (sizeof(char));
            NewDirRelativePath[0] = '\0';
        }



        //
        // Main processing:
        //


        // Get directory stream for iterating recursively all files.

        dirStream = opendir(DirFullPath);
        if (NULL == dirStream)
        {
            perror("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Could not execute opendir().\n");
            status = STATUS_WARNING;
            throw SyncDirException();
            // File may not exist anymore, or the user renamed/moved the file meanwhile.
        }


        // Loop. Until all files of the directory are iterated.
        // Update (or delete) every HashInfo of every file in the directory "dirStream".

        while (1)
        {


            // Get next directory entry (file).

            file = readdir(dirStream);
            if (NULL == file)
            {
                break;  // end of directory stream
            }    
            if (0 == strcmp(".", file->d_name) || 0 == strcmp("..", file->d_name))
            {
                continue;
            }


            // Get fstatat() with AT_SYMLINK_NOFOLLOW to not resolve symbolic links. So it behaves like lstat().

            if (fstatat(dirfd(dirStream), file->d_name, &fileStat, AT_SYMLINK_NOFOLLOW) < 0)
            {
                perror("[SyncDir] Warning: UpdateOrDeleteHashInfosForDirPath(): could not execute fstatat(). \n");
                printf("fstatat() warning was for file [%s].\n", file->d_name);
                                                                    // Possibly the file was (re)moved meanwhile. (Or altered.)
                continue;
            }



            // If file is non-directory.
            // Get HashInfo ==> Update or delete.

            if (!(S_ISDIR(fileStat.st_mode)))
            {


                sprintf(fileRelativePath, "%s/%s", DirRelativePath, file->d_name);
                sprintf(newFileRelativePath, "%s/%s", NewDirRelativePath, file->d_name);

                auxString.assign(fileRelativePath);             // Transform to std::string. Equivalent to operator=(const char *).

                auto it = HashInfoHMap.find(auxString);         // Get HashInfo of file.
                if (HashInfoHMap.end() == it)
                {
                    printf("[SyncDir] Warning: UpdateOrDeleteHashInfosForDirPath(): HashInfo not found for file [%s]. \n", fileRelativePath);
                    status = STATUS_WARNING;
                    continue;
                }

                oldHashInfo = & (it->second);



                // The UPDATE option.
                // I. Log: Check existence of HashInfo for corresponding hash code.
                // II. Insert the updated HashInfo for the new key (new file path) and for the hash code key.
                // III. Erase the HashInfo for the old key (old file path).

                if (0 == strcmp("UPDATE", UpdateOrDelete))
                {
                    // I.
                    if (HashInfoHMap.end() == HashInfoHMap.find(oldHashInfo->HashCode))
                    {
                        printf("[SyncDir] Info: UpdateOrDeleteHashInfosForDirPath(): HashInfo not found for hash key [%s] of file [%s].\n",
                            oldHashInfo->HashCode.c_str(), fileRelativePath);

                        //status = STATUS_WARNING;
                        // No warning because it can happen! (due to HashInfo insert policy; see InsertHashInfoOfFile() function).
                    }

                    // II.
                    status = InsertHashInfoOfFile(newFileRelativePath, oldHashInfo->HashCode.c_str(), oldHashInfo->FileSize, HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Failed at InsertHashInfoOfFile(). \n");
                        status = STATUS_FAIL;
                        throw SyncDirException();
                    }

                    // III.
                    status = DeleteHashInfoOfFile(oldHashInfo->FileRelativePath.c_str(), HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Warning: UpdateOrDeleteHashInfosForDirPath(): Failed at DeleteHashInfoOfFile() (at UPDATE). \n");
                        status = STATUS_WARNING;
                    }
                }



                // The DELETE option.
                // Delete the 2 HashInfo's (keys: hash code, file path).

                if (0 == strcmp("DELETE", UpdateOrDelete))
                {
                    status = DeleteHashInfoOfFile(oldHashInfo->FileRelativePath.c_str(), HashInfoHMap);
                    if (!(SUCCESS(status)))
                    {
                        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Failed at DeleteHashInfoOfFile() (at DELETE). \n");
                        status = STATUS_WARNING;
                        //throw SyncDirException();
                    }
                }
                


                // Consistency check. 

                if (0 != strcmp("UPDATE", UpdateOrDelete) && 0 != strcmp("DELETE", UpdateOrDelete))
                {
                    printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Bad UpdateOrDelete value.\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();
                }


            } //--> if (!(S_ISDIR(fileStat.st_mode)))



            // If file is directory.
            // Recursive call.

            if (S_ISDIR(fileStat.st_mode))
            {
                char nextDirFullPath[SD_MAX_PATH_LENGTH];
                char nextDirRelativePath[SD_MAX_PATH_LENGTH];
                char nextDirNewRelativePath[SD_MAX_PATH_LENGTH];

                sprintf(nextDirFullPath, "%s/%s", DirFullPath, file->d_name);                          // Form directory path to be explored.
                sprintf(nextDirRelativePath, "%s/%s", DirRelativePath, file->d_name);
                sprintf(nextDirNewRelativePath, "%s/%s", NewDirRelativePath, file->d_name);

                status = UpdateOrDeleteHashInfosForDirPath(nextDirFullPath, nextDirRelativePath, nextDirNewRelativePath, UpdateOrDelete, 
                    HashInfoHMap);
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Error at function recursive call.\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();                    
                }
            }            


        } // --> while(1)




        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: UpdateOrDeleteHashInfosForDirPath(): Unkown exception.\n");
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
        if (0 == strcmp("DELETE", UpdateOrDelete))
        {
            if (NULL != NewDirRelativePath)
            {
                free(NewDirRelativePath);
                NewDirRelativePath = NULL;
            }
        }        
    }
    else
    {
        if (NULL != dirStream)
        {
            closedir(dirStream);
            dirStream = NULL;
        }
        if (0 == strcmp("DELETE", UpdateOrDelete))
        {
            if (NULL != NewDirRelativePath)
            {
                free(NewDirRelativePath);
                NewDirRelativePath = NULL;
            }
        }
    }

    return status;
} // UpdateOrDeleteHashInfosForDirPath()



//
// UpdateHashInfoOfNondirFile
//
SDSTATUS
UpdateHashInfoOfNondirFile(
    __in char *FileRelativePath,
    __in char *NewFileRelativePath,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    )
/*++
Description: The routine updates the HASH_INFO structure of the file at path FileRelativePath. The file's HashInfo is updated with the new
file path NewFileRelativePath and made accessibile by the same new path.

- FileRelativePath: Pointer to the file relative path.
- NewFileRelativePath: Pointer to the new relative path of the file.
- HahsInfoHMap: Reference to the map containing all the HASH_INFO structures of all the files on the server.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).

--*/
{
    SDSTATUS    status;
    HASH_INFO   *oldHashInfo;
    std::string auxString;

    // PREINIT.

    status = STATUS_FAIL;
    oldHashInfo = NULL;

    // Parameter validation.

    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }    
    if (NULL == NewFileRelativePath || 0 == NewFileRelativePath[0])
    {
        printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }
    if (0 == HashInfoHMap.size())
    {
        printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Invalid parameter 3.\n");
        return STATUS_FAIL;        
    }  


    __try
    {
        // INIT.
        // --


        //
        // Main processing:
        //


        // Get HashInfo of the file path.

        auxString.assign(FileRelativePath);                             // Equivalent to "= FileRelativePath".

        auto it = HashInfoHMap.find(auxString);
        if (HashInfoHMap.end() == it)
        {
            printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): HashInfo to update was not found for file [%s].\n", FileRelativePath);
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        oldHashInfo = & (it->second);


        // Consistency check.

        if (0 != strcmp(FileRelativePath, oldHashInfo->FileRelativePath.c_str()))
        {
            printf("[SyncDir] Error (Fatal): UpdateHashInfoOfNondirFile(): Consistency error at HashInfo file path match. \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Insert new & Delete old.

        status = InsertHashInfoOfFile(NewFileRelativePath, oldHashInfo->HashCode.c_str(), oldHashInfo->FileSize, HashInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Failed at InsertHashInfoOfFile(). \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }

        status = DeleteHashInfoOfFile(oldHashInfo->FileRelativePath.c_str(), HashInfoHMap);
        if (!(SUCCESS(status)))
        {
            printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Failed at InsertHashInfoOfFile(). \n");
            status = STATUS_WARNING;
        }



        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        // "status" was set before throwing.
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: UpdateHashInfoOfNondirFile(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: UpdateHashInfoOfNondirFile(): Unkown exception.\n");
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
} // UpdateHashInfoOfNondirFile()




//
// DeleteHashInfoOfFile
//
SDSTATUS
DeleteHashInfoOfFile(
    __in const char *FileRelativePath,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    )
/*++
Description: The routine deletes from HashInfoHMap the HASH_INFO structure corresponding to the file FileRelativePath.
The routine complies with the HashInfo insert policy, described at InsertHashInfoOfFile() function. Therefore, a set of verifications and logs 
are made before the actual HashInfo deletion, as follows.
For the hash code key, the HashInfo existence is firstly verified because HashInfo might have been already deleted by a prior call to
DeleteHashInfoOfFile() with the same hash code (e.g. deletion for a file whose hash is the same, or for which the hash algorithm produced
a collision). Secondly, the HashInfo path match must be verified because the HashInfo at hash key might be of another file path, different
from the input FileRelativePath (due to same examples mentioned before). For more details, see HashInfo insert policy, at InsertHashInfoOfFile().

- FileRelativePath: Pointer to the relative path of the file (the path is relative to the main directory of the application).
- HahsInfoHMap: Reference to the map containing the HASH_INFO structures.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise.
--*/
{
    SDSTATUS        status;
    std::string     auxString;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: DeleteHashInfoOfFile(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (0 == HashInfoHMap.size())
    {
        printf("[SyncDir] Error: DeleteHashInfoOfFile(): Invalid parameter 2.\n");
        return STATUS_FAIL;        
    }  


    __try
    {
        // INIT.
        // --

        // Main processing:


        // Verify existence before deletion.

        auxString.assign(FileRelativePath);                             // Transform to std::string, for hash map search.

        auto it = HashInfoHMap.find(auxString);
        if (HashInfoHMap.end() == it)
        {
            printf("[SyncDir] Error: DeleteHashInfoOfFile(): HashInfo to delete was not found for path key [%s]. \n", FileRelativePath);
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Consistency check.

        if (0 != strcmp(FileRelativePath, it->second.FileRelativePath.c_str()))
        {
            printf("[SyncDir] Error (Fatal): DeleteHashInfoOfFile(): Consistency error at HashInfo file path match. \n");
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Delete HashInfo for the hash code key.
        // -> Conditions: 
        // 1. verify HashInfo existence.
        // 2. verify HashInfo path match (input vs file path at [HashCode]).
        // (the verifications are due to the HashInfo insert policy, described at InsertHashInfoOfFile() function.)

        if (HashInfoHMap.end() == HashInfoHMap.find(it->second.HashCode))                                               // Existence.
        {
            printf("[SyncDir] Info: DeleteHashInfoOfFile(): HashInfo missing for hash key [%s] of file [%s].\n",
                it->second.HashCode.c_str(), FileRelativePath);

            //status = STATUS_WARNING;
            // No warning because it can happen! (due to HashInfo insert policy; see InsertHashInfoOfFile() function).
        }
        else if (0 != strcmp(FileRelativePath, HashInfoHMap[it->second.HashCode].FileRelativePath.c_str()))             // Path match.
        {
            printf("[SyncDir] Info: DeleteHashInfoOfFile(): HashInfo's of same hash code do not match in path ([%s] vs [%s]).\n",
                FileRelativePath, HashInfoHMap[it->second.HashCode].FileRelativePath.c_str());

            //status = STATUS_WARNING;
            // No warning because it can happen! (due to HashInfo insert policy; see InsertHashInfoOfFile() function).
        }
        else if (1 != HashInfoHMap.erase(it->second.HashCode))                                                          // Deletion.
        {
            printf("[SyncDir] Error: DeleteHashInfoOfFile(): HashInfo could not be deleted for hash key [%s] of file [%s]. \n", 
                it->second.HashCode.c_str(), FileRelativePath);
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // Delete HashInfo for the path key.

        if (1 != HashInfoHMap.erase(it->second.FileRelativePath))
        {
            printf("[SyncDir] Error: DeleteHashInfoOfFile(): HashInfo could not be deleted for file path key [%s]. \n", 
                FileRelativePath);
            status = STATUS_FAIL;
            throw SyncDirException();
        }


        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(STATUS_SUCCESS);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        // "status" was set before throwing.        
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: DeleteHashInfoOfFile(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: DeleteHashInfoOfFile(): Unkown exception.\n");
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
} // DeleteHashInfoOfFile()




//
// InsertHashInfoOfFile
//
SDSTATUS
InsertHashInfoOfFile(
    __in const char     *FileRelativePath,
    __in const char     *HashCode,
    __in DWORD          FileSize,
    __inout std::unordered_map<std::string, HASH_INFO> & HashInfoHMap
    )
/*++
Description: The routine inserts in the map HashInfoHMap a HASH_INFO structure corresponding to the file at path FileRelativePath.
The new HASH_INFO contains the hash code HashCode and size FileSize.
The insertion in the hash map is made with two different keys - path and hash code - for constant O(1) access/search time. This helps
when only one value is known (either the path, or the hash code): e.g. when searching for identical file content (same hash) on the server.
In case of multiple files with same hash code, the value at HashInfoHMap[HashCode] is always replaced by the new inserted one.
This greedy technique reduces space usage and processing time, but has a drawback (see example below).

Note: Between its path and its hash code, a file is uniquely identified only by its path. The same hash code can be produced for two
different files, either because of the same file content, or because most hash algorithms may rarely produce collisions. However,
for time reasons, fast hash algorithms are often used and collisions are tolerated for their extremly low probabilities.

Example (redundant transfers):
- Setting: 
    Let hash(file_1 content) equal hash(file_2 content) and let HashInfo structure of file_2 be the last inserted in the HashInfoHMap.
    This means that the HashInfo of file_1 was overwritten by HashInfo of file_2, at location HashInfoHMap[hash(file_2 content)].
    Now, assume file_2 is deleted. Then, no HashInfo structure exists any more for file_1, nor for file_2, in the HashInfoHMap.
    In conclusion, the hash record for file_1 was overwritten and then deleted by means of file_2.
- Redundant transfer: 
    At this point, if a file_3 has to be transferred to the server, with hash(file_3) = hash(file_1), then the server will not find 
    any matching content hash (no value for hash(file_3) in the HashInfoHMap). So, file_3 will be completely transferred to the server, 
    which is redundant (because its content - file_1 - is already on the server).
- Solution: 
    One way to avoid these redundant transfers is to consider a multi-map for the HashInfo's and store all records of same hash code.
- Tradeoff:
    The benefit of having a HashInfo multi-map is counterbalanced by the collision probability of the hash algorithm.
    This is because the errors induced by the multi-map are proportional to the collision probability of the hash algorithm. However,
    the benefit of a multi-map is proportional to the number of identical files (as shown in the above example).
    Assuming the ideal scenario (no hash collisions - e.g. perfect hash algorithms), the tradeoff is just between the benefits of the
    HashInfo multi-map (no redundant file transfers) and its resource consumption (space usage, time).

Arguments:
- FileRelativePath: Pointer to the string containing the file relative path.
- HashCode: Pointer to the string containing the hash code of the file.
- FileSize: The size of the file, in bytes.
- HashInfoHMap: Reference to the hash map where the HASH_INFO structures are stored. 

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).

Note: Currently, the routine directly inserts the HashInfo at the HashCode key entry, replacing the existing value. Hence, in case of 
equal hash codes, this becomes a simple greedy approach, where the value at HashCode key will always be the last one inserted.
--*/
{
    SDSTATUS    status;
    HASH_INFO   newHashInfo;

    // PREINIT.

    status = STATUS_FAIL;

    // Parameter validation.

    if (NULL == FileRelativePath || 0 == FileRelativePath[0])
    {
        printf("[SyncDir] Error: InsertHashInfoOfFile(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == HashCode)
    {
        printf("[SyncDir] Error: InsertHashInfoOfFile(): Invalid parameter 2 \n");
        return STATUS_FAIL;
    }


    __try
    {
        // INIT.
        // --


        // Main processing:


        // Create the new HashInfo.

        newHashInfo.FileRelativePath = FileRelativePath;                                                    // operator= (const char *).
        newHashInfo.HashCode = HashCode;
        newHashInfo.FileSize = FileSize;


        // Insert at two key entries (path key and hash code key), to obtain O(1) access time when needed.
        // -> Insertion at HashCode key does is not affected if a value exists already. The possible existent value is replaced.
        // -> The value at HashCode will always be the newest inserted.

        auto pair = HashInfoHMap.insert({newHashInfo.FileRelativePath, newHashInfo});                      // key 1: path.
        if (FALSE == pair.second)
        {
            printf("[SyncDir] Warning: InsertHashInfoOfFile(): HashInfo insertion attempt (path key) was unsuccessful. Path key [%s]. \n",
                FileRelativePath);
            printf("A HashInfo is possible to already exist for this key. Overwriting ... \n");
            status = STATUS_WARNING;

            HashInfoHMap[newHashInfo.FileRelativePath] = newHashInfo;                                       // key 1: path. Overwrite.
        }

        pair = HashInfoHMap.insert({newHashInfo.HashCode, newHashInfo});                                   // key 2: hash code.
        if (FALSE == pair.second)
        {
            printf("[SyncDir] Info: InsertHashInfoOfFile(): HashInfo insertion attempt (hash key) was unsuccessful. Hash [%s], path [%s].\n",
                HashCode, FileRelativePath);
            printf("A HashInfo is possible to already exist for this key. Overwriting ... \n");
            //status = STATUS_WARNING;
            // No warning because it can happen! (due to HashInfo insert policy; see description above).

            HashInfoHMap[newHashInfo.HashCode] = newHashInfo;                                               // key 2: hash code. Overwrite.
        }        


        fprintf(g_SD_STDLOG, "[SyncDir] Info: New HashInfo added: \n - Relative path: [%s] \n - Hash code: [%s] \n",
            newHashInfo.FileRelativePath.c_str(), newHashInfo.HashCode.c_str());


        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(STATUS_SUCCESS);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: InsertHashInfoOfFile(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: InsertHashInfoOfFile(): Unkown exception.\n");
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
} // InsertHashInfoOfFile()





//
// BuildHashInfoForEachFile
//
SDSTATUS
BuildHashInfoForEachFile(
    __in const char     *DirFullPath,
    __in const char     *DirRelativePath,
    __out std::unordered_map<std::string, HASH_INFO> & HashInfoHMap    
    )
/*++
Description: The routine builds the map containing all the HASH_INFO structures of every file inside the DirFullPath directory and its
subdirectories. These structures are made accessible through the hash code of the file (e.g. using MD5 algorithm) and thorugh the file 
relative path.

- DirFullPath: Reference to the string containing the full path of the directory.
- DirRelativePath: Reference to the string containing the relative path of the directory.
- HahsInfoHMap: Reference to the map where the routine stores all the HASH_INFO structures.

Return value: STATUS_SUCCESS on success, STATUS_FAIL otherwise. STATUS_WARNING may be returned if the main purpose of the routine
was achieved, but related issues were encountered (information is logged, thereby).
--*/
{
    SDSTATUS        status;
    DIR             *dirStream;
    struct dirent   *file;
    struct stat     fileStat;
    char            hashCode[SD_HASH_CODE_LENGTH+1];
    char            fileFullPath[SD_MAX_PATH_LENGTH];


    // PREINIT.

    status = STATUS_FAIL;
    dirStream = NULL;
    file = NULL;
    hashCode[0] = 0;

    // Parameter validation.

    if (NULL == DirFullPath || 0 == DirFullPath[0])
    {
        printf("[SyncDir] Error: BuildHashInfoForEachFile(): Invalid parameter 1.\n");
        return STATUS_FAIL;
    }
    if (NULL == DirRelativePath || 0 == DirRelativePath[0])
    {
        printf("[SyncDir] Error: BuildHashInfoForEachFile(): Invalid parameter 2.\n");
        return STATUS_FAIL;
    }

    __try
    {
        // INIT.
        // --
                
        // Get directory stream for iterating recursively all files.

        dirStream = opendir(DirFullPath);
        if (NULL == dirStream)
        {
            perror("[SyncDir] Error: BuildHashInfoForEachFile(): could not execute opendir().\n");
            status = STATUS_WARNING;
            throw SyncDirException();
            // File may not exist anymore, or the user renamed/moved the file meanwhile.
        }


        // Iterate the subdirectories/subfiles of the parent directory (== dirStream).

        while (1)
        {
            hashCode[0] = 0;                                                     // Init.


            // >Get next directory entry.

            file = readdir(dirStream);
            if (NULL == file)
            {
                break;                                                          // End of directory stream.
            }    
            if (0 == strcmp(".", file->d_name) || 0 == strcmp("..", file->d_name))
            {
                continue;
            }


            // Get fstatat() with AT_SYMLINK_NOFOLLOW to not resolve symbolic links. So it behaves like lstat().

            if (fstatat(dirfd(dirStream), file->d_name, &fileStat, AT_SYMLINK_NOFOLLOW) < 0)
            {
                perror("[SyncDir] Warning: BuildHashInfoForEachFile(): could not execute fstatat(). \n");
                printf("fstatat() warning was for the file [%s].\n", file->d_name);
                                                                                // Possibly the file was (re)moved meanwhile. (Or altered.)
                continue;
            }


            // Get current file full path.

            sprintf(fileFullPath, "%s/%s", DirFullPath, file->d_name);


            // If file is non-directory.
            // Build new HashInfo.
            // Store the new HashInfo.

            if (!(S_ISDIR(fileStat.st_mode)))
            {
                HASH_INFO   hashInfo;
                char        formatPath[SD_MAX_PATH_LENGTH];


                snprintf(formatPath, SD_MAX_PATH_LENGTH, "%s/%s", DirRelativePath, file->d_name);
                hashInfo.FileRelativePath.assign(formatPath);                       // equivalent to operator=(const char*)
                hashInfo.FileSize = fileStat.st_size;

                status = MD5HashOfFile(fileFullPath, hashCode);                      // Note: For now, use MD5 hash algorithm for each file.
                if (!(SUCCESS(status)))
                {
                    printf("[SyncDir] Warning: BuildHashInfoForEachFile(): Hash code function failed for file [%s].\n", fileFullPath);
                                                                                    // Maybe file was altered (deleted) meanwhile.
                    status = STATUS_WARNING;     
                    continue;             
                }

                hashInfo.HashCode.assign(hashCode);                                 // equivalent to operator=(const char*)

                HashInfoHMap[hashInfo.HashCode] = hashInfo;
                HashInfoHMap[hashInfo.FileRelativePath] = hashInfo;                 // For quick access by file path.
                                                                                    // Average access time: O(1). Excluding O(hash).

                fprintf(g_SD_STDLOG, "[SyncDir] Info: Added HashInfo: \n - hash code [%s], \n - relative path [%s], \n - file size [%u]. \n",
                    hashInfo.HashCode.c_str(), hashInfo.FileRelativePath.c_str(), hashInfo.FileSize);

                continue;
            }


            // If file is directory.
            // Recursive call.

            if (S_ISDIR(fileStat.st_mode))
            {
                char tempFullPath[SD_MAX_PATH_LENGTH];
                char tempRelativePath[SD_MAX_PATH_LENGTH];

                sprintf(tempFullPath, "%s/%s", DirFullPath, file->d_name);
                sprintf(tempRelativePath, "%s/%s", DirRelativePath, file->d_name);

                fprintf(g_SD_STDLOG, "[SyncDir] Info: Recursive call: Create HashInfo's for files inside: \n - directory full path [%s] \n"
                    " - directory relative path [%s] \n", tempFullPath, tempRelativePath);

                status = BuildHashInfoForEachFile(tempFullPath, tempRelativePath, HashInfoHMap);
                if (!(SUCCESS(status)))
                {
                    perror("[SyncDir] Error: BuildHashInfoForEachFile(): Error at function recursive call.\n");
                    status = STATUS_FAIL;
                    throw SyncDirException();                    
                }                
                continue;
            }

        } // --> while(1)

        // If here, everything is ok.
        status = SUCCESS_KEEP_WARNING(status);

    } // --> __try
    __catch (const SyncDirException &e)
    {
        cout << e.what() << "\n";        
        //status = STATUS_FAIL;
    }
    __catch (const std::exception &e)
    {
        cout << "[SyncDir] Error: BuildHashInfoForEachFile(): Standard Exception caught: " << e.what() << "\n";        
        status = STATUS_FAIL;
    }
    __catch (...)
    {
        printf("[SyncDir] Error: BuildHashInfoForEachFile(): Unkown exception.\n");
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
} // BuildHashInfoForEachFile()



