**************************
SSC Strong Staging Coupler
**************************

The SSC engine is designed specifically for strong code coupling. Currently SSC only supports fixed IO pattern, which means once the first step is finished, users are not allowed to write or read a data block with a *start* and *count* that have not been written or read in the first step. SSC uses a combination of one sided MPI and two sided MPI methods. In any cases, all user applications are required to be launched within a single mpirun or mpiexec command, using the MPMD mode.

The SSC engine takes the following parameters:

1. ``OpenTimeoutSecs``: Default **10**. Timeout in seconds for opening a stream. The SSC engine's open function will block until the RendezvousAppCount is reached, or timeout, whichever comes first. If it reaches the timeout, SSC will throw an exception.

2. ``MpiMode``: Default **TwoSided**. MPI communication modes to use. Besides the default TwoSided mode using two sided MPI communications, MPI_Isend and MPI_Irecv, for data transport, there are four one sided MPI modes: OneSidedFencePush, OneSidedPostPush, OneSidedFencePull, and OneSidedPostPull. Modes with **Push** are based on the push model and use MPI_Put for data transport, while modes with **Pull** are based on the pull model and use MPI_Get. Modes with **Fence** use MPI_Win_fence for synchronization, while modes with **Post** use MPI_Win_start, MPI_Win_complete, MPI_Win_post and MPI_Win_wait.

=============================== ================== ================================================
 **Key**                         **Value Format**   **Default** and Examples
=============================== ================== ================================================
 OpenTimeoutSecs                        integer             **10**, 2, 20, 200
 MpiMode                                string             **TwoSided**, OneSidedFencePush, OneSidedPostPush, OneSidedFencePull, OneSidedPostPull
=============================== ================== ================================================


