from mpi4py import MPI
import numpy as np
import adios2
import os

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

configFile = './defaultConfig.xml'
queryFile = './sampleQuery.xml'
dataPath = './heat.bp'


def doAnalysis(reader, touched_blocks, varList):
    print(" Step: ", reader.CurrentStep(),
          "  num touched blocks: ", len(touched_blocks))
    values = []
    data = {}

    for var in varList:
        data[var] = []

    if (len(touched_blocks) > 0):
        for n in touched_blocks:
            for var in varList:
                values = np.zeros(n[1], dtype=np.double)
                var.SetSelection(n)
                reader.Get(var, values, adios2.Mode.Sync)
                data[var].extend(values)
                # do analysis with data here


def runQuery():
    adios = adios2.ADIOS(configFile, comm, True)
    queryIO = adios.DeclareIO("query")
    reader = queryIO.Open(dataPath, adios2.Mode.Read, comm)
    w = adios2.Query(queryFile, reader)

    touched_blocks = []

    print("Num steps: ", reader.Steps())

    while (reader.BeginStep() == adios2.StepStatus.OK):
        # say only rank 0 wants to process result
        var = [queryIO.InquireVariable("T")]

        if (rank == 0):
            touched_blocks = w.GetResult()
            doAnalysis(reader, touched_blocks, var)

    reader.EndStep()
    reader.Close()


def createConfigFile():
    print(".. Writing config file to: ", configFile)
    file1 = open(configFile, 'w')

    xmlContent = ["<?xml version=\"1.0\"?>\n",
                  "<adios-config>\n",
                  "<io name=\"query\">\n",
                  "  <engine type=\"BPFile\">\n",
                  "  </engine>\n",
                  "  <transport type=\"File\">\n",
                  "    <parameter key=\"Library\" value=\"POSIX\"/>\n",
                  "  </transport>\n",
                  "</io>\n", "</adios-config>\n"]

    file1.writelines(xmlContent)
    file1.close()


def createQueryFile():
    print(".. Writing query file to: ", queryFile)

    file1 = open(queryFile, 'w')
    queryContent = [
        "<?xml version=\"1.0\"?>\n", "<adios-query>\n",
        "  <io name=\"query\">\n"
        "  <var name=\"T\">\n",
        "    <op value=\"AND\">\n",
        "      <range  compare=\"LT\" value=\"2.7\"/>\n",
        "      <range  compare=\"GT\" value=\"2.66\"/>\n", "    </op>\n",
        "  </var>\n", "  </io>\n", "</adios-query>\n"
    ]
    file1.writelines(queryContent)
    file1.close()


if (os.path.exists(dataPath) is False):
    print("Please generate data file:", dataPath,
          " from heat transfer example first.")
else:
    # configFile created
    createConfigFile()

    # queryFile Generated
    createQueryFile()

    print(".. Running query against: ", dataPath)
    runQuery()

    print("Now clean up.")
    os.remove(queryFile)
    os.remove(configFile)
