from __future__ import absolute_import, division, print_function, unicode_literals
import time
import numpy as np
from os.path import basename, exists, isdir
from os import fstat
import bp4dbg_utils


def ReadEncodedString(f, ID, limit):
    # 2 bytes length + string without \0
    namelen = np.fromfile(f, dtype=np.uint16, count=1)[0]
    if (namelen > limit):
        print("ERROR: " + ID + " string length ({0}) is longer than the limit to stay inside the block ({1})".format(
            namelen, limit))
        return ""
    name = f.read(namelen)
    return name


def ReadHeader(f):
    # There is no header at this time in data.*\
    print("Header info: There is no header in data.*")


def ReadCharacteristicsFromData(f, limit, typeID):
    # 1 byte NCharacteristics
    nCharacteristics = np.fromfile(f, dtype=np.uint8, count=1)[0]
    print("      # of Characteristics    : {0}".format(nCharacteristics))
    # 4 bytes length
    charLen = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("      Characteristics Length  : {0}".format(charLen))

    for i in range(nCharacteristics):
        print("      Characteristics[{0}]".format(i))
        # 1 byte TYPE
        cID = np.fromfile(f, dtype=np.uint8, count=1)[0]
        print("          Type        : {0} ({1}) ".format(
            bp4dbg_utils.GetCharacteristicName(cID), cID))
        cLen = bp4dbg_utils.GetCharacteristicDataLength(cID, typeID)
        cData = f.read(cLen)
        print("          Content     : {0} bytes = {1}".format(
            cLen, cData))
    return True


def ReadVarData(f, nElements, typeID, ldims, varsStartPosition, varsTotalLength):
    typeSize = bp4dbg_utils.GetTypeSize(typeID)
    if (typeSize == 0):
        print("ERROR: Cannot process variable data block with unknown type size")
        return False

    currentPosition = f.tell()
    print("      Var payload offset: {0}".format(currentPosition))
    nBytes = np.ones(1, dtype=np.uint64)
    nBytes[0] = nElements[0] * typeSize
    if (currentPosition + nBytes[0] > varsStartPosition + varsTotalLength):
        print("ERROR: Variable data block of size would reach beyond all variable blocks")
        print("VarsStartPosition = {0} varsTotalLength = {1}".format(
            varsStartPosition, varsTotalLength))
        print("current Position = {0} var block length = {1}".format(
            currentPosition, nBytes[0]))
        # return False
    print("      Variable Data   : {0} bytes".format(nBytes[0]))
    data = f.read(nBytes[0])
    return True


def ReadVMD(f, varidx, varsStartPosition, varsTotalLength):
    print("  Var {0:5d}".format(varidx))
    print("      Starting offset : {0}".format(f.tell()))
    # 4 bytes TAG
    tag = f.read(4)
    if (tag != b"[VMD"):
        print("  Tag: " + str(tag))
        print("ERROR: VAR group does not start with [VMD")
        return False
    print("      Tag             : " + tag.decode('ascii'))

    currentPosition = f.tell()
    # 8 bytes VMD Length
    vmdlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("      Var block size  : {0} bytes (+4 for Tag)".format(vmdlen))
    expectedVMDLength = vmdlen - 8  # need to read this more
    print("VarsStartPosition = {0} varsTotalLength = {1}".format(
        varsStartPosition, varsTotalLength))
    print("current Position = {0} var block length = {1}".format(
        currentPosition, expectedVMDLength))
    if (currentPosition + expectedVMDLength > varsStartPosition + varsTotalLength):
        print("ERROR: There is not enough bytes inside this PG to read this Var block")

        return False

    # 4 bytes VAR MEMBER ID
    memberID = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("      Member ID       : {0}".format(memberID))

    # VAR NAME, 2 bytes length + string without \0
    varname = ReadEncodedString(f, "Var Name", expectedVMDLength)
    if (varname == ""):
        return False
    print("      Var Name        : " + varname.decode('ascii'))

    # VAR PATH, 2 bytes length + string without \0
    varpath = ReadEncodedString(f, "Var Path", expectedVMDLength)
    if (varname == ""):
        return False
    print("      Var Path        : " + varpath.decode('ascii'))

    # 1 byte TYPE
    typeID = np.fromfile(f, dtype=np.uint8, count=1)[0]
    print("      Type            : {0} ({1}) ".format(
        bp4dbg_utils.GetTypeName(typeID), typeID))

    # ISDIMENSIONS 1 byte, 'y' or 'n'
    isDimensionVar = f.read(1)
    if (isDimensionVar != b'y' and isDimensionVar != b'n'):
        print(
            "ERROR: Next byte for isDimensionVar must be 'y' or 'n' but it isn't = {0}".format(isDimension))
        return False
    print("      isDimensionVar  : " + isDimensionVar.decode('ascii'))

    # 1 byte NDIMENSIONS
    ndims = np.fromfile(f, dtype=np.uint8, count=1)[0]
    print("      # of Dimensions : {0}".format(
        ndims))

    # DIMLENGTH
    dimsLen = np.fromfile(f, dtype=np.uint16, count=1)[0]
    print("      Dims Length     : {0}".format(
        dimsLen))

    nElements = np.ones(1, dtype=np.uint64)
    ldims = np.zeros(ndims, dtype=np.uint64)
    for i in range(ndims):
        print("      Dim[{0}]".format(i))
        # Read Local Dimensions (1 byte flag + 8 byte value)
        # Is Dimension a variable ID 1 byte, 'y' or 'n' or '\0'
        isDimensionVarID = f.read(1)
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and isDimensionsVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' but it isn't = {0}".format(isDimension))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        ldims[i] = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           local  dim : {0}".format(ldims[i]))
        nElements = nElements * ldims[i]
        # Read Global Dimensions (1 byte flag + 8 byte value)
        # Is Dimension a variable ID 1 byte, 'y' or 'n' or '\0'
        isDimensionVarID = f.read(1)
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and isDimensionsVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' but it isn't = {0}".format(isDimension))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        gdim = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           global dim : {0}".format(gdim))

        # Read Offset Dimensions (1 byte flag + 8 byte value)
        # Is Dimension a variable ID 1 byte, 'y' or 'n' or '\0'
        isDimensionVarID = f.read(1)
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and isDimensionsVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' but it isn't = {0}".format(isDimension))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        offset = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           offset dim : {0}".format(offset))

    status = ReadCharacteristicsFromData(f, expectedVMDLength, typeID)
    if (not status):
        return False

    # Padded end TAG
    # 1 byte length of tag
    endTagLen = np.fromfile(f, dtype=np.uint8, count=1)[0]
    tag = f.read(endTagLen)
    if (not tag.endswith(b"VMD]")):
        print("  Tag: " + str(tag))
        print("ERROR: VAR group metadata does not end with VMD]")
        return False
    print("      Tag (pad {0:2d})    : {1}".format(
        endTagLen - 4, tag.decode('ascii')))

    status = ReadVarData(f, nElements, typeID, ldims,
                         varsStartPosition, varsTotalLength)
    if (not status):
        return False

    return True


def ReadPG(f, fileSize):
    print("PG: ")
    print("  Starting offset : {0}".format(f.tell()))
    # 4 bytes TAG
    tag = f.read(4)
    if (tag != b"[PGI"):
        print("  Tag: " + str(tag))
        print("ERROR: PG group does not start with [PGI")
        return False

    print("  Tag             : " + tag.decode('ascii'))

    # 8 bytes PG Length
    pglen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("  PG length       : {0} bytes (+4 for Tag)".format(pglen))
    expectedPGLength = pglen - 8  # need to read this more
    if (f.tell() + expectedPGLength > fileSize):
        print("ERROR: There is not enough bytes in file to read this PG")
        return False

    # RowMajor 1 byte, 'y' or 'n'
    isRowMajor = f.read(1)
    if (isRowMajor != b'y' and isRowMajor != b'n'):
        print(
            "ERROR: Next byte for isRowMajor must be 'y' or 'n' but it isn't = {0}".format(isRowMajor))
        return False
    print("  isRowMajor      : " + isRowMajor.decode('ascii'))

    # PG Name, 2 bytes length + string without \0
    pgname = ReadEncodedString(f, "PG Name", expectedPGLength)
    if (pgname == ""):
        return False
    print("  PG Name         : " + pgname.decode('ascii'))

    # 4 bytes unused (for Coordination variable)
    tag = f.read(4)
    print("  Unused 4 bytes  : " + str(tag))

    # Timestep name
    tsname = ReadEncodedString(f, "Timestep Name", expectedPGLength)
    if (tsname == ""):
        return False
    print("  Step Name       : " + tsname.decode('ascii'))

    # STEP 4 bytes
    step = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("  Step Value      : {0}".format(step))

    # Method Count 1 byte1
    nMethods = np.fromfile(f, dtype=np.uint8, count=1)[0]
    print("  Methods count   : {0}".format(nMethods))

    # Method Length 2 byte1
    lenMethods = np.fromfile(f, dtype=np.uint16, count=1)[0]
    print("  Methods length  : {0}".format(lenMethods))

    print("  Methods info")
    for i in range(nMethods):
        # Method ID
        methodID = np.fromfile(f, dtype=np.uint8, count=1)[0]
        print("      Method ID   : {0}".format(nMethods))
        methodParams = ReadEncodedString(
            f, "Method Parameters", expectedPGLength)
        if (methodParams == ""):
            return False
        print('      M. params   : "' +
              methodParams.decode('ascii') + '"')

    # VARS COUNT 4 bytes
    nVars = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("  # of Variables  : {0}".format(nVars))

    # VARS SIZE 8 bytes
    varlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("  Vars length     : {0} bytes".format(varlen))
    expectedVarsLength = varlen  # need to read this more
    if (expectedVarsLength > expectedPGLength):
        print("ERROR: There is not enough bytes in PG to read the variables")
        return False

    varsStartPosition = f.tell()
    for i in range(nVars):
        # VMD block
        status = ReadVMD(f, i, varsStartPosition, expectedVarsLength)
        if (not status):
            return False

    return True


def DumpData(fileName):
    print("=== Data File: " + fileName + " ====")
    with open(fileName, "rb") as f:
        fileSize = fstat(f.fileno()).st_size
        status = True
        while (f.tell() < fileSize - 12 and status):
            status = ReadPG(f, fileSize)


if __name__ == "__main__":
    print("ERROR: Utility main program is bp4dbg.py")
