####################
Campaign Management
####################

`Campaign Management <https://hpc-campaign.readthedocs.io>`_ is a separate toolkit, and is used for collecting basic information and metadata about a collection of ADIOS2/HDF5 output files, images and text files, from a single application run or multiple runs, on one or multiple hosts. The campaign archive is a single file (.ACA) that can be transferred to other locations and shared with others interested in reading the collected data. 

The .ACA campaign file can be opened by ADIOS2 and all the metadata can be processed (including the values of GlobalValue and LocalValue variables, or min/max of each Arrays at each step and decomposition/min/max of each block in an Array at each step). However, Get() operations will only succeed to read actual data of the arrays, if the data belonging to the campaign is either local or some mechanism for remote data access to the location of the data is set up in advance.

.. warning::

    Campaign Management is fairly new, currently at version 0.5. It will change substantially in the future and campaign files produced by this version will have to be updated to newer versions. Make sure to use a compatible versions of ADIOS2 and hpc-campaign.

Requirements
============
The Campaign Reader engine uses **SQlite3** and **ZLIB** for its operations and have to be turned on at configuration of ADIOS2 (`-DADIOS2_USE_Campaign=ON` for cmake). Check `bpls -Vv` to see if `CAMPAIGN` is in the list of "Available features".

Caching data requires having a Redis key-value database running and the **hiredis** API available when building ADIOS2. Add the location of the hiredis library to `-DCMAKE_PREFIX_PATH` for cmake. 

Limitations
===========

- The Campaign Reader engine only supports ReadRandomAccess mode, not step-by-step reading. Campaign management will need to change in the future to support sorting the steps from different outputs to a coherent order. 
- Updates to moving data for other location is not supported yet

Example
-------

The Gray-Scott example, that is included with ADIOS2, in `examples/simulation/gray-scott`, has two programs, Gray-Scott and PDF-Calc. The first one produces the main output `gs.bp` which includes the main 3D variables `U` and `V`, and a checkpoint file `ckpt.bp` with a single step in it. PDF-Calc processes the main output and produces histograms on 2D slices of U and V (`U/bins` and `U/pdf`) in `pdf.bp`. A campaign can include all the three output files as they logically belong together. 

Assuming we are in 

- /lustre/orion/csc143/proj-shared/demo/gray-scott
- on a machine that we named **OLCF** in our Campaign hostname in ~/.config/hpc-campaign/config.yaml
- our campaignpath is set to **/lustre/orion/csc143/proj-shared/adios-campaign-store/demoproject** 

.. code-block:: bash
		
    # run application as usual
    $ mpirun -n 4 adios2_simulations_gray-scott settings-files.json
    $ ls -d *.bp
    ckpt.bp gs.bp

    $ hpc_campaign manager demoproject/frontier_gray-scott_100 delete --campaign
    $ hpc_campaign manager demoproject/frontier_gray-scott_100 create
    $ hpc_campaign manager demoproject/frontier_gray-scott_100 dataset gs.bp ckpt.bp
    
    $ mpirun -n 3 adios2_simulations_gray-scott_pdf-calc gs.bp pdf.bp 1000
    $ ls -d *.bp
    ckpt.bp gs.bp pdf.bp

    $ hpc_campaign manager demoproject/frontier_gray-scott_100 dataset pdf.bp
    $ hpc_campaign manager demoproject/frontier_gray-scott_100 text settings-files.json --store

    $ hpc_campaign manager demoproject/frontier_gray-scott_100 info
    =============================
    ADIOS Campaign Archive, version 0.5, created on Sep 10 14:29

    Hosts and directories:
      OLCF   longhostname = frontier05341.frontier.olcf.ornl.gov
        1. /lustre/orion/csc143/proj-shared/demo/gray-scott

    Other Datasets:
        3a4bf0b14cc33424a470862bd67ed007  ADIOS  Sep 10 14:25   ckpt.bp
        0fce4b1173f432f7ae5d2282df9077a6  ADIOS  Sep 10 14:25   gs.bp
        b42d0da4a0793adca341ace1ff6e628d  ADIOS  Sep 10 14:28   pdf.bp
        85a0b724b22f37a4a79ad8a0cf1127d1  TEXT   Sep 10 14:24   settings-files.json

    # The campaign archive is small compared to the data it points to 
    $ du -sh *bp
    263K    ckpt.bp
    385M    gs.bp
    104K    pdf.bp

    $ du -sh /lustre/orion/csc143/proj-shared/adios-campaign-store/demoproject/frontier_gray-scott_100.aca
    97K     /lustre/orion/csc143/proj-shared/adios-campaign-store/demoproject/frontier_gray-scott_100.aca

    # ADIOS can list the content of the campaign archive
    $ bpls -l demoproject/frontier_gray-scott_100
        double   ckpt.bp/U      {4, 34, 34, 66} = 0.171103 / 1
        double   ckpt.bp/V      {4, 34, 34, 66} = 1.71086e-19 / 0.438921
        int32_t  ckpt.bp/step   scalar = 700
        double   gs.bp/U        100*{64, 64, 64} = 0.0908114 / 1
        double   gs.bp/V        100*{64, 64, 64} = 0 / 0.674804
        int32_t  gs.bp/step     100*scalar = 10 / 1000
        double   pdf.bp/U/bins  100*{1000} = 0.0908235 / 1
        double   pdf.bp/U/pdf   100*{64, 1000} = 0 / 4096
        double   pdf.bp/V/bins  100*{1000} = 0 / 0.67413
        double   pdf.bp/V/pdf   100*{64, 1000} = 0 / 4096
        int32_t  pdf.bp/step    100*scalar = 10 / 1000
        char     settings-files.json  {440} = A / Z

    # scalar over steps is available in metadata
    $ bpls -l demoproject/frontier_gray-scott_100 -d pdf.bp/step -n 10
      int32_t  pdf.bp/step    10*scalar = 100 / 1000
        ( 0)    10 20 30 40 50 60 70 80 90 100
        (10)    110 120 130 140 150 160 170 180 190 200
        (20)    210 220 230 240 250 260 270 280 290 300
        (30)    310 320 330 340 350 360 370 380 390 400
        (40)    410 420 430 440 450 460 470 480 490 500
        (50)    510 520 530 540 550 560 570 580 590 600
        (60)    610 620 630 640 650 660 670 680 690 700
        (70)    710 720 730 740 750 760 770 780 790 800
        (80)    810 820 830 840 850 860 870 880 890 900
        (90)    910 920 930 940 950 960 970 980 990 1000

    # Array decomposition including min/max are available in metadata
    $ bpls -l demoproject/frontier_gray-scott_100 -D gs.bp/V
      double   gs.bp/V        10*{64, 64, 64} = 8.24719e-63 / 0.515145
        step 0:
          block 0: [ 0:63,  0:31,  0:31] = 0 / 0.600691
          block 1: [ 0:63, 32:63,  0:31] = 0 / 0.600691
          block 2: [ 0:63,  0:31, 32:63] = 0 / 0.600691
          block 3: [ 0:63, 32:63, 32:63] = 0 / 0.600691
        ...
        step 99:
          block 0: [ 0:63,  0:31,  0:31] = 3.99938e-09 / 0.441838
          block 1: [ 0:63, 32:63,  0:31] = 3.99946e-09 / 0.441802
          block 2: [ 0:63,  0:31, 32:63] = 3.99966e-09 / 0.44183
          block 3: [ 0:63, 32:63, 32:63] = 3.99955e-09 / 0.441833

    # Array data is only available if data is local
    $ ./bin/bpls -l demoproject/frontier_gray-scott_100 -d pdf.bp/U/bins -n 10 -c "1,-1"
      double   pdf.bp/U/bins  100*{1000} = 0.0908235 / 1
        slice (0:0, 0:999)
        (0,  0)    0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992
        ...
        (0,990)    1 1 1 1 1 1 1 1 1 1

    $ ./bin/bpls -l demoproject/frontier_gray-scott_100 -d pdf.bp/U/bins -n 10 -s "-1,0" -c "1,-1"
      double   pdf.bp/U/bins  100*{1000} = 0.0908235 / 1
        slice (99:99, 0:999)
        (0,  0)    0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992 0.999992
        ...
        (0,990)    1 1 1 1 1 1 1 1 1 1


Remote access
=============
For now, we have one way to access data, through SSH port forwarding and running a remote server program to read in data on the remote host and to send back the data to the local ADIOS program. `adios2_remote_server` is included in the adios installation. You need to use the one built on the host.

Assuming the campaign archive was synced to a local machine's campaign store under `csc143/demoproject`, now we can look at some of the content:

.. code-block:: bash

    $ hpc_campaign list gray-scott
    csc143/demoproject/frontier_gray-scott_100.aca

    $ bpls -l csc143/demoproject/frontier_gray-scott_100.aca
      double   ckpt.bp/U            {4, 34, 34, 66} = 0.171103 / 1
      ...
      char     settings-files.json  {440} = A / Z

    # data stored inside the campaign can be read easily
    $ bpls -l csc143/demoproject/frontier_gray-scott_100.aca -d pdf.bp/step
      int32_t  pdf.bp/step    10*scalar = 100 / 1000
        ( 0)    10 20 30 40 50 60 70 80 90 100
        ...
        (90)    910 920 930 940 950 960 970 980 990 1000

    $ bpls -l csc143/demoproject/frontier_gray-scott_100.aca -dyS settings-files.json
      ; char     settings-files.json  {440} = A / Z
      "{
          "L": 64,
          ...
          "mesh_type": "image"
      }
      "

To read array data though, we need to set up remote data access. On the local machine set up *~/.config/hpc-campaign/hosts.yaml* so that the campaign connector can find how to connect to **OLCF**.

Assuming that 

- I am user *user007* at OLCF
- installed adios2 into *~/dtn/sw/adios2*

.. code-block:: bash

    $ cat ~/.config/hpc-campaign/hosts.yaml
    OLCF:
      dtn-ssh:
          protocol: ssh
          host: dtn.olcf.ornl.gov
          user: user007
          authentication: passcode
          serverpath: ~/dtn/sw/adios2/bin/adios2_remote_server
          args: -background -report_port_selection -v -v -l ~/dtn/log.adios2_remote_server -t 16
          verbose: 1

First, we need to launch the **hpc_campaign connector**, specifying to load the host configuration, and to listen on port `30000` for the requests for connections.

.. code-block:: bash
		
    $ hpc_campaign connector -c ~/.config/hpc-campaign/hosts.yaml -p  30000
    SSH Tunnel Server:  127.0.0.1 30000

Assuming the campaign archive was synced to a local machine's campaign store under `csc143/demoproject`, now we can retrieve data:

.. code-block:: bash

    # array data is requested from the remote server
    # read 16 values (4x4x4) from U from last step, from offset 30,30,30
    $ bpls -l csc143/demoproject/frontier_gray-scott_100.aca  -d gs.bp/U -s "-1,30,30,30" -c "1,4,4,4" -n 4
    double   gs.bp/U              100*{64, 64, 64} = 0.0908114 / 1
      slice (99:99, 30:33, 30:33, 30:33)
      (99,30,30,30)    0.891887 0.899848 0.899847 0.891884
      (99,30,31,30)    0.899851 0.908275 0.908275 0.899849
      (99,30,32,30)    0.899852 0.908276 0.908276 0.89985
      (99,30,33,30)    0.89189 0.899851 0.899851 0.891889
      (99,31,30,30)    0.899848 0.908273 0.908272 0.899845
      (99,31,31,30)    0.908275 0.916976 0.916975 0.908273
      (99,31,32,30)    0.908276 0.916977 0.916976 0.908274
      (99,31,33,30)    0.899851 0.908275 0.908275 0.899849
      (99,32,30,30)    0.899847 0.908272 0.908271 0.899844
      (99,32,31,30)    0.908275 0.916976 0.916975 0.908272
      (99,32,32,30)    0.908275 0.916976 0.916976 0.908273
      (99,32,33,30)    0.89985 0.908274 0.908274 0.899848
      (99,33,30,30)    0.891886 0.899846 0.899845 0.891882
      (99,33,31,30)    0.89985 0.908274 0.908273 0.899847
      (99,33,32,30)    0.89985 0.908275 0.908274 0.899848
      (99,33,33,30)    0.891888 0.899849 0.899849 0.891886


This array data should be listed after the connection manager pops up a window asking for
the passcode to login to OLCF, and logs on screen activity similar to this:

.. code-block:: bash

    $ hpc_campaign connector -c ~/.config/hpc-campaign/hosts.yaml -p  30000
    SSH Tunnel Server:  127.0.0.1 30000
    Client 127.0.0.1:
    Request  : /run_service?group=OLCF&service=dtn-ssh
    Parsed Request:  {'group': ['OLCF'], 'service': ['dtn-ssh']}
    Remote service request:  {'group': ['OLCF'], 'service': ['dtn-ssh']}
    ...
    Connecting to remote server dtn.olcf.ornl.gov:22 ...
    Service command: ~/dtn/sw/adios2/bin/adios2_remote_server -background -report_port_selection -v -v -l ~/dtn/log.adios2_remote_server -t 16
    Parsing service response...
    LINE:  port:58547;msg:no_error;cookie:0xd93d91e3643c9869

    SERVICE DATA:  {'port': '58547', 'msg': 'no_error', 'cookie': '0xd93d91e3643c9869'}
    Service data: {'port': '58547', 'msg': 'no_error', 'cookie': '0xd93d91e3643c9869'}
    Checking if port 28000 is available.
    Opening tunnel for local port 28000 to dtn.olcf.ornl.gov:58547
    Got the forward server
    Starting.
    Connected!  Tunnel open ('127.0.0.1', 50492) -> ('160.91.195.184', 22) -> ('dtn.olcf.ornl.gov', 58547)

