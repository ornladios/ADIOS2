from codar.cheetah import Campaign
from codar.cheetah import parameters as p
from codar.cheetah.parameters import SymLink
import copy


class NWChem(Campaign):
    # A name for the campaign
    name = "nwchem"

    # Define your workflow. Setup the applications that form the workflow.
    # exe may be an absolute path.
    # The adios xml file is automatically copied to the campaign directory.
    # 'runner_override' may be used to launch the code on a login/service node as a serial code
    #   without a runner such as aprun/srun/jsrun etc.
    codes = [ ("nwchem_main", dict(exe="/ccs/proj/e2e/pnorbert/ADIOS/ADIOS2/build.rhea.gcc/install/bin/adios2_iotest", adios_xml_file='copro.xml')),
              ("sorting", dict(exe="/ccs/proj/e2e/pnorbert/ADIOS/ADIOS2/build.rhea.gcc/install/bin/adios2_iotest", adios_xml_file='copro.xml', runner_override=False)), ]

    # List of machines on which this code can be run
    supported_machines = ['local', 'titan', 'theta', 'rhea']

    # Kill an experiment right away if any workflow components fail (just the experiment, not the whole group)
    kill_on_partial_failure = True

    # Any setup that you may need to do in an experiment directory before the experiment is run
    run_dir_setup_script = None

    # A post-process script that is run for every experiment after the experiment completes
    run_post_process_script = None

    # Directory permissions for the campaign sub-directories
    umask = '027'

    # Options for the underlying scheduler on the target system. Specify the project ID and job queue here.
    # scheduler_options = {'theta': {'project':'CSC249ADCD01', 'queue': 'default'}}
    scheduler_options = {'rhea': {'project':'csc143'}}

    # A way to setup your environment before the experiment runs. Export environment variables such as LD_LIBRARY_PATH here.
    app_config_scripts = {'local': 'setup.sh', 'theta': 'env_setup.sh', 'rhea': 'setup_nwchem_rhea.sh'}

    # Setup the sweep parameters for a Sweep
    sweep1_parameters = [
            # ParamRunner 'nprocs' specifies the no. of ranks to be spawned 
            p.ParamRunner       ('nwchem_main', 'nprocs', [80]),

            # Create a ParamCmdLineArg parameter to specify a command line argument to run the application
            p.ParamCmdLineOption   ('nwchem_main', 'app', '-a', [1]),
            p.ParamCmdLineOption   ('nwchem_main', 'app-config', '-c', ['copro-80.txt']),
            p.ParamCmdLineOption   ('nwchem_main', 'adios-config', '-x', ['copro.xml']),
            p.ParamCmdLineOption   ('nwchem_main', 'strongscaling', '-w', [None]),
            p.ParamCmdLineOption   ('nwchem_main', 'timing', '-t', [None]),
            p.ParamCmdLineOption   ('nwchem_main', 'decomposition', '-d', [80]),
            # Change the engine for the 'SimulationOutput' IO object in the adios xml file to SST for coupling.
            p.ParamADIOS2XML    ('nwchem_main', 'dump_trajectory', 'trj_dump_out', 'engine', [ {'BP4':{'OpenTimeoutSecs':'30.0'}} ]),
            # Now setup options for the pdf_calc application.
            # Sweep over four values for the nprocs 
            p.ParamRunner       ('sorting', 'nprocs', [8]),
            p.ParamCmdLineOption   ('sorting', 'app', '-a', [1]),
            p.ParamCmdLineOption   ('sorting', 'app-config', '-c', ['copro-80.txt']),
            p.ParamCmdLineOption   ('sorting', 'adios-config', '-x', ['copro.xml']),
            p.ParamCmdLineOption   ('sorting', 'weakscaling', '-s', [None]),
            p.ParamCmdLineOption   ('sorting', 'timing', '-t', [None]),
            p.ParamCmdLineOption   ('sorting', 'decomposition', '-d', [8]),
            # Change the engine for the 'SimulationOutput' IO object in the adios xml file to SST for coupling.
            p.ParamADIOS2XML    ('sorting', 'load_trajectory', 'trj_dump_in', 'engine', [ {'BP4':{'OpenTimeoutSecs':'30.0'}} ]),

    ]
    #print(sweep1_parameters)
    sweep2_parameters = sweep1_parameters.copy()
    sweep2_parameters[7] = p.ParamADIOS2XML('nwchem_main', 'dump_trajectory', 'trj_dump_out', 'engine', [ {'SST':{}} ])
    sweep2_parameters[15] = p.ParamADIOS2XML('sorting', 'load_trajectory', 'trj_dump_in', 'engine', [ {'SST':{}} ])
    sweep3_parameters = sweep1_parameters.copy()
    sweep3_parameters[7] = p.ParamADIOS2XML('nwchem_main', 'dump_trajectory', 'trj_dump_out', 'engine', [ {"Null":{}} ])
    sweep3_parameters[15] = p.ParamADIOS2XML('sorting', 'load_trajectory', 'trj_dump_in', 'engine', [ {"Null":{}} ])
    sweep4_parameters = sweep1_parameters.copy()
    sweep4_parameters[7] = p.ParamADIOS2XML('nwchem_main', 'dump_trajectory', 'trj_dump_out', 'engine', [ {'BP4':{'OpenTimeoutSecs':'30.0', 'BurstBufferPath':'/tmp'}} ])
    sweep4_parameters[15] = p.ParamADIOS2XML('sorting', 'load_trajectory', 'trj_dump_in', 'engine', [ {'BP4':{'OpenTimeoutSecs':'30.0', 'BurstBufferPath':'/tmp'}} ])
    #sweep4_parameters = sweep1_parameters.copy()
    #sweep4_parameters[7] = p.ParamADIOS2XML('nwchem_main', 'dump_trajectory', 'trj_dump_out', 'engine', [ {'SSC':{'DataTransport':'WAN'}} ])
    #sweep4_parameters[15] = p.ParamADIOS2XML('sorting', 'load_trajectory', 'trj_dump_in', 'engine', [ {'SSC':{'DataTransport':'WAN'}} ])

    # Create Sweep objects. This one does not define a node-layout, and thus, all cores of a compute node will be 
    #   utilized and mapped to application ranks.
    sweep1 = p.Sweep (parameters = sweep1_parameters)
    sweep2 = p.Sweep (parameters = sweep2_parameters)
    sweep3 = p.Sweep (parameters = sweep3_parameters)
    sweep4 = p.Sweep (parameters = sweep4_parameters)

    # Create a SweepGroup and add the above Sweeps. Set batch job properties such as the no. of nodes, 
    sweepGroup1 = p.SweepGroup ("nwchem-adios", # A unique name for the SweepGroup
                                walltime=18060,  # Total runtime for the SweepGroup
                                per_run_timeout=500,    # Timeout for each experiment                                
                                parameter_groups=[sweep1, sweep2, sweep3, sweep4],   # Sweeps to include in this group
                                launch_mode='default',  # Launch mode: default, or MPMD if supported
                                nodes=6,  # No. of nodes for the batch job.
                                run_repetitions=2,  # No. of times each experiment in the group must be repeated (Total no. of runs here will be 3)
                                component_inputs = {'nwchem_main': ['copro-80.txt']},
                                )
    
    # Activate the SweepGroup
    sweeps = [sweepGroup1]

