**************************
SSC Strong Staging Coupler
**************************

The SSC engine is designed specifically for strong code coupling. Currently SSC only supports fixed IO pattern, which means once the first step is finished, users are not allowed to write or read a data block with a *start* and *count* that have not been written or read in the first step. SSC uses a combination of one sided MPI and two sided MPI methods. In any cases, all user applications are required to be launched within a single mpirun or mpiexec command, using the MPMD mode.

The SSC engine takes the following parameters:

1. ``RendezvousAppCount``: Default **2**. The number of applications, including both writers and readers, that will work on this stream. The SSC engine's open function will block until all these applications reach the open call. If there are multiple applications in a workflow, this parameter needs to be set respectively for every application. For example, in a three-app coupling scenario: App 0 writes Stream A to App 1; App 1 writes Stream B to App 0; App 2 writes Stream C to App 1; App 1 writes Stream D to App 2, the parameter RendezvousAppCount for engine instances of every stream should be all set to 2, because for each of the streams, two applications will work on it. In another example, where App 0 writes Stream A to App 1 and App 2; App 1 writes Stream B to App 2, the parameter RendezvousAppCount for engine instances of Stream A and B should be set to 3 and 2 respectively, because three applications will work on Stream A, while two applications will work on Stream B.

2. ``MaxStreamsPerApp``: Default **1**. The maximum number of streams that all applications sharing this MPI_COMM_WORLD can possibly open. It is required that this number is consistent across all ranks from all applications. This is used for pre-allocating the vectors holding MPI handshake informations and due to the fundamental communication mechanism of MPI, this information must be set statically through engine parameters, and the SSC engine cannot provide any mechanism to check if this parameter is set correctly. If this parameter is wrongly set, the SSC engine's open function will either exit early than expected without gathering all applications' handshake information, or it will block until timeout. It may cause other unpredictable errors too.

3. ``OpenTimeoutSecs``: Default **10**. Timeout in seconds for opening a stream. The SSC engine's open function will block until the RendezvousAppCount is reached, or timeout, whichever comes first. If it reaches the timeout, SSC will throw an exception.

4. ``MaxFilenameLength``: Default **128**. The maximum length of filenames across all ranks from all applications. It is used for allocating the handshake buffer. Due to the limitation of MPI communication, this number must be set statically. The default number should work for most use cases. SSC will throw an exception if any rank opens a stream with a filename longer than this number.

5. ``MpiMode``: Default **TwoSided**. MPI communication modes to use. Besides the default TwoSided mode using two sided MPI communications, MPI_Isend and MPI_Irecv, for data transport, there are four one sided MPI modes: OneSidedFencePush, OneSidedPostPush, OneSidedFencePull, and OneSidedPostPull. Modes with **Push** are based on the push model and use MPI_Put for data transport, while modes with **Pull** are based on the pull model and use MPI_Get. Modes with **Fence** use MPI_Win_fence for synchronization, while modes with **Post** use MPI_Win_start, MPI_Win_complete, MPI_Win_post and MPI_Win_wait.

=============================== ================== ================================================
 **Key**                         **Value Format**   **Default** and Examples
=============================== ================== ================================================
 RendezvousAppCount                     integer             **2**, 3, 5, 10
 MaxStreamsPerApp                       integer             **1**, 2, 4, 8
 OpenTimeoutSecs                        integer             **10**, 2, 20, 200
 MaxFilenameLength                      integer             **128**, 32, 64, 512
 MpiMode                                string             **TwoSided**, OneSidedFencePush, OneSidedPostPush, OneSidedFencePull, OneSidedPostPull
=============================== ================== ================================================


