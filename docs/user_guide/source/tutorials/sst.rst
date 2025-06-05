.. _sec:tutorials_sst:
====================
SST Engine Example:
====================
    - programs are from example/hello/ : 
        - reader =>  bin/adios2_hello_sstReader
        - writer =>  bin/adios2_hello_sstWriter


0. Notes:
"""""""""""""""""
    - It is easier to test in interactive node on HPC
    - usage: writer [DataTransportChoice] (defaults to WAN) 
    - To turn on verbose flag for both SST server or client, set this in your env:
         - export SstVerbose=N ( 0 <= N <= 5)
         - To check whether the preferred SST Dataplane is activated, look for "DataTransport" values in the verbose output.

1. WAN (default)
"""""""""""""""""
    - works for both serial or MPI code.
    - using sockets to communicate
        - srun -n 1 -N 1  writer > wout 2>&1 &
        - srun -n 1 -N 1  reader > rout 2>&1 &

2. RDMA
*************** 
    - add option **"--network=single_node_vni,job_vni"**
    - working example:
        - srun -n 5 -N 1  --network=single_node_vni,job_vni writer RDMA > wout 2>&1 &
        - srun -n 2 -N 1  --network=single_node_vni,job_vni reader > rout 2>&1 &

3. MPI
***************
 - needs 2+ nodes on server
 - add option **"--network=single_node_vni,job_vni"**
 - working example: 
      - srun -n 2 -N 2 --network=single_node_vni,job_vni writer MPI > wout 2>&1 &
      - srun -n 2 -N 2 --network=single_node_vni,job_vni reader > rout 2>&1 &

4. UCX
***************
    
    - on Frontier, to use UCX: 
       - Export UCX_ROOT=/opt/cray/pe/cray-ucx/2.7.0-1/ucx/
       - load 3 ucx modules:
            - craype-network-ucx  
            - cray-mpich-ucx/8.1.31 
            - cray-ucx/2.7.0-1
       -  (May 2025) compile ADIOS (with ucx), cmake linked to a non exist ucx installation. To work around, change CMakeCache.txt and force to use /opt/cray/pe/cray-ucx/2.7.0-1/ucx instead of the wrong path. 
       - 1 Node works (as of May 2025)
            - srun -N 1 -n 2 ./adios2_hello_bpWriter_mpi
       - 2 Node failed (as of May 2025)
            - MPIDI_UCX_mpi_init_hook(139):  ucx function returned with failed status(ucx_init.c 139 MPIDI_UCX_mpi_init_hook Destination is unreachable)


 
