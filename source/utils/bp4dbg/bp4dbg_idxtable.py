from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import numpy as np
from os import fstat
import bp4dbg_utils


def ReadIndexHeader(f, fileSize):
    status = True
    if (fileSize < 64):
        print("ERROR: Invalid Index Table. File is smaller "
              "than the index header (64 bytes)")
        return False
    header = f.read(64)
    hStr = header.decode('ascii')

    versionStr = hStr[0:24].replace('\0', ' ')
    major = hStr[24]
    minor = hStr[25]
    micro = hStr[26]
#    unused = hStr[27]

    endianValue = header[28]
    if endianValue == 0:
        endian = 'yes'
    elif endianValue == 1:
        endian = 'no'
    else:
        print("ERROR: byte 28 must be 0 or 1 to indicate endianness of "
              "the data. It is however {0} in this file".format(
                  endianValue))
        status = False

#    subfile1 = header[29]
    subfile2 = header[30]
    if subfile2 == 0:
        hasSubfiles = 'no'
    else:
        hasSubfiles = 'yes'

    bpversion = int(header[31])

    # 32..55 unused

    live = np.frombuffer(header, dtype=np.uint64, count=1, offset=56)
    if live == 0:
        isLive = 'no'
    elif live == 1:
        isLive = 'yes'
    else:
        print(
            "ERROR: bytes 57-64 must be an uint64 integer with value "
            "0 or 1 to indicate if the data is still being producer. "
            "It is however {0} in this file".format(live))
        status = False

    print("------------------------------------------------------------"
          "----------------------------------------------------")
    print("|      Version string      | Major | Minor | Micro | unused "
          "| Endian | Subfiles | BP version | unused | isLive |")
    print("|          24 bytes        |   1B  |   1B  |   1B  |   1B   "
          "|   1B   |    2B    |     1B     |   24B  |   8B   |")
    print("+-----------------------------------------------------------"
          "---------------------------------------------------+")
    print(
        "| {0} |   {1}   |   {2}   |   {3}   |        |  {4}   |    {5}   |"
        "      {6}     |        |   {7}  |"
        .format(versionStr, major, minor, micro, endian, hasSubfiles,
                bpversion, isLive))
    print("------------------------------------------------------------"
          "----------------------------------------------------")
    return status


def ReadIndex(f, fileSize):
    nBytes = fileSize - f.tell()
    if nBytes <= 0:
        return True
    nRows = int(nBytes / 64)
    table = f.read(nBytes)
    print(" ")
    print("-----------------------------------------------------------------"
          "------------------------------------------")
    print("|   Step   |   Rank   |    PGPtr    |   VarPtr    |   AttPtr    |"
          "   EndPtr    |  Timestamp  |    unused   |")
    print("+----------------------------------------------------------------"
          "-----------------------------------------+")
    for r in range(0, nRows):
        pos = r * 64
        data = np.frombuffer(table, dtype=np.uint64, count=8, offset=pos)
        step = str(data[0]).rjust(9)
        rank = str(data[1]).rjust(9)
        pgptr = str(data[2]).rjust(12)
        varptr = str(data[3]).rjust(12)
        attptr = str(data[4]).rjust(12)
        endptr = str(data[5]).rjust(12)
        time = str(data[6]).rjust(12)
        unused = str(data[7]).rjust(12)
        print("|" + step + " |" + rank + " |" + pgptr + " |" + varptr + " |" +
              attptr + " |" + endptr + " |" + time + " |" + unused + " |")

    print("-----------------------------------------------------------------"
          "------------------------------------------")

    if fileSize - f.tell() > 1:
        print("ERROR: There are {0} bytes at the end of file"
              " that cannot be interpreted".format(fileSize - f.tell() - 1))
        return False

    return True


def DumpIndexTable(fileName):
    print("========================================================")
    print("    Index Table File: " + fileName)
    print("========================================================")
    status = False
    with open(fileName, "rb") as f:
        fileSize = fstat(f.fileno()).st_size
        status = bp4dbg_utils.ReadHeader(f, fileSize, "Index Table")
        if (status):
            status = ReadIndex(f, fileSize)
    return status


if __name__ == "__main__":
    print("ERROR: Utility main program is bp4dbg.py")
