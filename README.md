

* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0



README: SyncDir tool (Version 3.1).

______________
Description:

Real-time synchronization of file system directories and partitions.

SyncDir v3.1 synchronizes the contents of a directory tree structure, between a SyncDir client application and a SyncDir server application. The synchronization is performed with respect to all file content present in the SyncDir client directories. All the file content events that occur on the SyncDir client are reflected in the SyncDir server directories. SyncDir configuration allows for both instantaneous and delayed synchronization. This means that the server can be updated at the time of the events or after a period of time, if desired. A brief description of current features can be found below (see "Current Implementation Features" section).

______________
Prerequisites:

- Unix-like system with POSIX.
- Kernel library: "Inotify".
- C 2011 standard, C++ 2011 standard (gcc -x c -std=gnu11; gcc -x c++ -std=c++11).
- Other languages: AWK standard.
- Command line tools: md5sum.


________________________
Compilation and Linkage:

1. Open a terminal in the SyncDir root directory.
2. Recommended: To avoid usage of already compiled objects, execute "make clean" before compiling.
3. Compile sources.

- Compilation option 1: execute the following command in shell:

make
=> "make" will compile and link both server and client programs. Produces the executables: "SyncDir_Client" and "SyncDir_Server".

- Compilation option 2: compile and link client program and server program independently:

make client
=> Executable produced: "SyncDir_Client".

make server
=> Executable produced: "SyncDir_Server".
 	

_______
Launch:

I. Recommendation: To avoid any sort of conflict at start-up synchronization, it is recommended to have an empty main directory on the SyncDir server (i.e. the <dir_path_srv> argument, found below).


II. Pre-defined examples: 

Execute run_example_srv.sh and then execute run_example_clt.sh, in shell (or modify their contents before execution):
sh run_example_srv.sh
sh run_example_clt.sh


III. User defined launch:

- Server: ./SyncDir_Server <port> <dir_path_srv>

- Client: ./SyncDir_Client <port> <ip_address> <dir_path_clt> <shutdown_time>

Where:
 
<port> - Is the port that the server waits on for client connections. The same port has to be passed as argument for both the client and the server. It is recommended to set a port between 49152 and 65535.
<ip_address> - Is the human readable "x.x.x.x" IP address of the server interface that the clients should connect to.
<dir_path_srv> - Is the path where the server performs the content updates received from the client. It has to be different from the dir_path_clt, if both server and client run on the same local machine;
<dir_path_clt> - Is the path of the partition/directory which the client monitors for changes.
<shutdown_time> - Is the time (in seconds) after which the SyncDir client will shut down, interrupting the connection with the server. A value of 0 will set the shut down time to infinity.


IV. Launch examples:

./bin/SyncDir_Server 65432 SYNCDIR_test_srv/dir_xxx

./bin/SyncDir_Client 65432 127.0.0.1 SYNCDIR_test_clt/dir_xxx 0


____________________________
Built-in Parametrization:

- To set a minimum waiting time before the synchronization is performed (i.e. server update), modify the value of SD_MIN_TIME_BEFORE_SYNC, in the header of syncdir_clt_def_types.h. Its value denotes the number of seconds that SyncDir client app before any information is sent to the server. Currently, this value is set to 0, for a quick server update.

        In syncdir_clt_def_types.h :
        #define SD_MIN_TIME_BEFORE_SYNC 0

- To set a different time treshold that SyncDir client may use to wait for side-events while processing the Inotify event queue, modify the value of SD_TIME_TRESHOLD_AT_SYNC. The SyncDir client selects a random number of seconds in [0, SD_TIME_TRESHOLD_AT_SYNC] to wait for possible side-events, after it has emptied the Inotify event queue. An example of such an event is a MOVED_TO operation that is generated after a MOVED_FROM operation and appears later on, as a result of a whole MOVE operation. Currently, SD_TIME_TRESHOLD_AT_SYNC is set to 5 seconds.

        In syncdir_clt_def_types.h :
        #define SD_TIME_TRESHOLD_AT_SYNC 5

- To set a different number of initial directory watch structures (e.g. in case of large directory trees):

        In syncdir_clt_def_types.h :
        #define SD_INITIAL_NR_OF_WATCHES 50

- To set a different size (in bytes) for the file chunks transferred from client to server:

        In syncdir_essential_def_types.h :
        #define SD_PACKET_DATA_SIZE 1024

- To set a different maximum path length (in bytes):

        In syncdir_essential_def_types.h :
        #define SD_MAX_PATH_LENGTH 4096

- To set a different maximum filename length (in bytes):

        In syncdir_essential_def_types.h :
        #define SD_MAX_FILENAME_LENGTH 256


________________________
General Recommendations:

- For "SyncDir_Client" launch: It is recommended to set a waiting time before updating the server (SD_TIME_TRESHOLD_AT_SYNC) of at least 5 seconds (or even 10 seconds) to permit the SyncDir_Client to take advantage of the implemented features of "events aggregation for file recovery" e.g. when a file is accidentally deleted by the user, or accidentally moved. This prevents redundant file transfers to the server, as well as unnecessary communication.


________________________________
Current Implementation Features:
(v.3.1)

- Recursive deep synchronization: Quick transfer of file changes, for large directory trees / partitions.
- Capture of operations such as FIL_MOVED_FROM, DIR_MOVED_FROM, FIL_MOVED_TO, DIR_MOVED_TO, FIL_MOVE, DIR_MOVE, FIL_DELETE, DIR_DELETE, FIL_CREATE, DIR_CREATE, MODIFY.
- Event aggregation: Building event records for every file and relations between them. Most of the aggregation of one file's events takes place in the same record, but it can affect other records if a relation was established in between. Since a lot of successive file operations can occur in the system, the event aggregation also includes a simplification/reduction of the event file records. 
- Minimum time before server update: Event aggregation feature creates the possibility of setting a waiting time before the server is synchronized with the client. This is because aggregating events offers the benefit of delayed synchronization (i.e. not requiring instant or immediate server update).
- Side-events treshold: Time treshold that SyncDir client may use to wait for side-events while processing the kernel event queue. An application of this feature is "cut & paste" operations (where the new file location can appear later on in a side-event), or a "delete" operation made by mistake, where the "undo" performed by the user would be quickly taken into account and no redundant transfer is thereby spent.
- Redundant transfer avoidance: Avoiding transfers of files that are already on the server. This is done by recording hash codes of all the file content on the server and checking for matches before any file transfer.
- Pre-commit filtered update: When the stage of event aggregation is performed, all the SyncDir client structures are updated independently, and the modifications are stored separately for every file. This creates the possibility of filtering the updates (w.r.t. server filters) or snapshot-ing certain modified directories, if desired.
- Symbolic link detection, treatment and validation.
- Fault tolerance for corrupted files or files overwritten while in transfer.

Induced features:
Event aggregation + Side-events treshold ==> The usage of these two features together enhanced the file transfer and execution time in the case of temporary or volatile files (e.g. backup files) created usually by text editors, while editing files.

Possible SyncDir updates (v.3.2):
- Capture of FIL_IN_ATTRIB, DIR_IN_ATTRIB related operations.
- Enhancing the SyncDir server data structure to preserve a subset of all the file paths with the same hash code, not only the most recent file path. 

_________
Notes:

Terminology:
- "Relative path" of a file refers to a path relative to the main directory (directory to synchronize), for both server and client. For SyncDir implementation, the relative path of a file on the server application is the same as on the client application.
- "Full path" of a file refers to a path starting from the root directory "/" (absolute path), but which may contain links, so it is not necessarily a real path towards the file (i.e. resolved absolute path).
- "Real path" of a file refers to the resolved absolute path of the file, i.e. the canonicalized absolute path. All its subpaths are resolved, so they contain no symbolic links.


_______________________
"SYNCDIR_notes" Folder:

- For alternatives to SyncDir source implementation: see SYNCDIR_alternatives.txt.

- For notes on the SyncDir current implementation + few minor possible optimizations: see SYNCDIR_notes.txt.





