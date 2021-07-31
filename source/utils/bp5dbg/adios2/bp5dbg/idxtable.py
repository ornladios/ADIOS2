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
        print("|" + rank + " | FlushCount = " + sub + " |")
    print("=====================")
    return True

def ReadIndex(f, fileSize, WriterCount):
    nBytes = fileSize - f.tell()
    if nBytes <= 0:
        return True
    table = f.read(nBytes)
    pos = 0
    step = 0
    while pos < nBytes:
        print("-----------------------------------------------" +
              "---------------------------------------------------")
        data = np.frombuffer(table, dtype=np.uint64, count=3,
                             offset=pos)
        stepstr = str(step).ljust(6)
        mdatapos = str(data[0]).ljust(10)
        mdatasize = str(data[1]).ljust(10)
        flushcount = str(data[2]).ljust(3)
        FlushCount = data[2]
        print("|   Step = " + stepstr + "| MetadataPos = " + mdatapos +
              " |  MetadataSize = " + mdatasize + "   |" + flushcount + "|")

        pos = pos + 3 * 8
        for Writer in range(0, WriterCount):
            start = " Writer " + str(Writer) + " data "
            thiswriter = np.frombuffer(table, dtype=np.uint64,
                                       count=int(FlushCount * 2 + 1),
                                       offset=pos)

            for i in range(0, FlushCount):  # for flushes
                start += ("loc:" + str(thiswriter[int(i * 2)]) + " siz:" +
                          str(thiswriter[i * 2 + 1]) + "; ")
            start += "loc:" + str(thiswriter[int(FlushCount * 2)])
            pos = int(pos + (FlushCount * 2 + 1) * 8)
            print(start)
        print("---------------------------------------------------" +
              "-----------------------------------------------")
        step = step + 1

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
