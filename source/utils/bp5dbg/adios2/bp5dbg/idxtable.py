import numpy as np
from os import fstat
from .utils import *

WriterCount = -1

def ReadWriterArray(f, fileSize, WriterCount):

    print("Writer count is " + str(WriterCount))
    array = f.read(WriterCount * 8)
    print("=====================")
    print("|  Rank  |  Subfile |")
    print("=====================")
    for r in range(0, WriterCount):
        pos = r * 8
        data = np.frombuffer(array, dtype=np.uint64, count=1, offset=pos)
        rank = str(r).rjust(7)
        sub = str(data[0]).rjust(9)
        print("|" + rank + " |" + sub + " |")
    print("=====================")
    return True

def ReadIndex(f, fileSize, WriterCount):
    nBytes = fileSize - f.tell()
    if nBytes <= 0:
        return True
    nRows = int(nBytes / (8 * (2 + WriterCount)))
    table = f.read(nBytes)
    print(" timestep count is " + str(nRows))
    for r in range(0, nRows):
        pos = r * 8 * (2 + WriterCount)
        data = np.frombuffer(table, dtype=np.uint64, count=2 + WriterCount,
                             offset=pos)
        step = str(r).ljust(6)
        mdatapos = str(data[0]).ljust(10)
        mdatasize = str(data[1]).ljust(10)
        print("-----------------------------------------------" +
              "---------------------------------------------------")
        print("|   Step = " + step + "| MetadataPos = " + mdatapos +
              " |  MetadataSize = " + mdatasize + "   |")
        covered = 0
        for s in range(0, int(WriterCount / 5)):
            for t in range(0, 5):
                start = ""
                start = start + str(data[covered + t + 2]).rjust(10)
            print("Data Pos")
            print("| Ranks " + str(covered) + "-" + str(covered + 4) +
                  " " + start)
            covered = covered + 5
        covered = int(WriterCount / 5) * 5
        remainder = WriterCount - covered
        for t in range(0, remainder):
            start = ""
            start = start + str(data[covered + t + 2]).rjust(10)
        print(" Ranks " + str(covered) + "-" + str(covered + remainder - 1) +
              " " + start)
    print("---------------------------------------------------" +
          "-----------------------------------------------")

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
        status = ReadHeader(f, fileSize, "Index Table")
        if isinstance(status, list):
            WriterCount = status[1]
            status = status[0]
        if status:
            status = ReadWriterArray(f, fileSize, WriterCount)
        if status:
            status = ReadIndex(f, fileSize, WriterCount)
    return status


if __name__ == "__main__":
    print("ERROR: Utility main program is bp5dbg.py")
