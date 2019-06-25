from __future__ import (absolute_import, division, print_function,
                        unicode_literals)
import numpy as np
from os import fstat
import bp4dbg_utils


def ReadEncodedString(f, ID, limit):
    # 2 bytes length + string without \0
    namelen = np.fromfile(f, dtype=np.uint16, count=1)[0]
    if (namelen > limit):
        print("ERROR: " + ID + " string length ({0}) is longer than the "
              "limit to stay inside the block ({1})".format(
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


# Read Variable data
def ReadVarData(f, nElements, typeID, ldims, expectedSize,
                varsStartPosition, varsTotalLength):
    typeSize = bp4dbg_utils.GetTypeSize(typeID)
    if (typeSize == 0):
        print("ERROR: Cannot process variable data block with "
              "unknown type size")
        return False

    currentPosition = f.tell()
    print("      Payload offset  : {0}".format(currentPosition))
    nBytes = np.ones(1, dtype=np.uint64)
    nBytes[0] = nElements[0] * typeSize
    if (currentPosition + nBytes[0] > varsStartPosition + varsTotalLength):
        print("ERROR: Variable data block of size would reach beyond all "
              "variable blocks")
        print("VarsStartPosition = {0} varsTotalLength = {1}".format(
            varsStartPosition, varsTotalLength))
        print("current Position = {0} var block length = {1}".format(
            currentPosition, nBytes[0]))
        # return False
    if (nBytes[0] != expectedSize):
        print("ERROR: Variable data block size does not equal the size "
              "calculated from var block length")
        print("Expected size = {0}  calculated size from dimensions = {1}".
              format(expectedSize, nBytes[0]))
    print("      Variable Data   : {0} bytes".format(nBytes[0]))
    f.read(nBytes[0])
    return True

# Read a variable's metadata


def ReadVMD(f, varidx, varsStartPosition, varsTotalLength):
    startPosition = f.tell()
    print("  Var {0:5d}".format(varidx))
    print("      Starting offset : {0}".format(startPosition))
    # 4 bytes TAG
    tag = f.read(4)
    if (tag != b"[VMD"):
        print("  Tag: " + str(tag))
        print("ERROR: VAR group does not start with [VMD")
        return False
    print("      Tag             : " + tag.decode('ascii'))

    # 8 bytes VMD Length
    vmdlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("      Var block size  : {0} bytes (+4 for Tag)".format(vmdlen))
    expectedVarBlockLength = vmdlen + 4  # [VMD is not included in vmdlen

    if (startPosition + expectedVarBlockLength >
            varsStartPosition + varsTotalLength):
        print("ERROR: There is not enough bytes inside this PG to read "
              "this Var block")
        print("VarsStartPosition = {0} varsTotalLength = {1}".format(
            varsStartPosition, varsTotalLength))
        print("current var's start position = {0} var block length = {1}".
              format(startPosition, expectedVarBlockLength))
        return False

    # 4 bytes VAR MEMBER ID
    memberID = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("      Member ID       : {0}".format(memberID))

    # VAR NAME, 2 bytes length + string without \0
    sizeLimit = expectedVarBlockLength - (f.tell() - startPosition)
    varname = ReadEncodedString(f, "Var Name", sizeLimit)
    if (varname == ""):
        return False
    print("      Var Name        : " + varname.decode('ascii'))

    # VAR PATH, 2 bytes length + string without \0
    sizeLimit = expectedVarBlockLength - (f.tell() - startPosition)
    varpath = ReadEncodedString(f, "Var Path", sizeLimit)
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
            "ERROR: Next byte for isDimensionVar must be 'y' or 'n' "
            "but it isn't = {0}".format(isDimensionVar))
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
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and
                isDimensionVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' "
                "but it isn't = {0}".format(isDimensionVarID))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        ldims[i] = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           local  dim : {0}".format(ldims[i]))
        nElements = nElements * ldims[i]
        # Read Global Dimensions (1 byte flag + 8 byte value)
        # Is Dimension a variable ID 1 byte, 'y' or 'n' or '\0'
        isDimensionVarID = f.read(1)
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and
                isDimensionVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' "
                "but it isn't = {0}".format(isDimensionVarID))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        gdim = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           global dim : {0}".format(gdim))

        # Read Offset Dimensions (1 byte flag + 8 byte value)
        # Is Dimension a variable ID 1 byte, 'y' or 'n' or '\0'
        isDimensionVarID = f.read(1)
        if (isDimensionVarID != b'y' and isDimensionVarID != b'n' and
                isDimensionVarID != b'\0'):
            print(
                "ERROR: Next byte for isDimensionVarID must be 'y' or 'n' "
                "but it isn't = {0}".format(isDimensionVarID))
            return False
        if (isDimensionVarID == b'\0'):
            isDimensionVarID = b'n'
        offset = np.fromfile(f, dtype=np.uint64, count=1)[0]
        print("           offset dim : {0}".format(offset))

    sizeLimit = expectedVarBlockLength - (f.tell() - startPosition)
    status = ReadCharacteristicsFromData(f, sizeLimit, typeID)
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

    expectedVarDataSize = expectedVarBlockLength - (f.tell() - startPosition)
    status = ReadVarData(f, nElements, typeID, ldims, expectedVarDataSize,
                         varsStartPosition, varsTotalLength)
    if (not status):
        return False

    return True

# Read an attribute's metadata and value


def ReadAMD(f, attridx, attrsStartPosition, attrsTotalLength):
    startPosition = f.tell()
    print("  attr {0:5d}".format(attridx))
    print("      Starting offset : {0}".format(startPosition))
    # 4 bytes TAG
    tag = f.read(4)
    if (tag != b"[AMD"):
        print("  Tag: " + str(tag))
        print("ERROR: ATTR group does not start with [AMD")
        return False
    print("      Tag             : " + tag.decode('ascii'))

    # 8 bytes VMD Length
    vmdlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("      Attr block size : {0} bytes (+4 for Tag)".format(vmdlen))
    expectedAttrBlockLength = vmdlen + 4  # [VMD is not included in vmdlen
    print("AttrsStartPosition = {0} attrsTotalLength = {1}".format(
        attrsStartPosition, attrsTotalLength))
    print("current attr's start position = {0} attr block length = {1}".format(
        startPosition, expectedAttrBlockLength))
    if (startPosition + expectedAttrBlockLength >
            attrsStartPosition + attrsTotalLength):
        print("ERROR: There is not enough bytes inside this PG "
              "to read this Attr block")
        return False

    # 4 bytes ATTR MEMBER ID
    memberID = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("      Member ID       : {0}".format(memberID))

    # ATTR NAME, 2 bytes length + string without \0
    sizeLimit = expectedAttrBlockLength - (f.tell() - startPosition)
    attrname = ReadEncodedString(f, "Attr Name", sizeLimit)
    if (attrname == ""):
        return False
    print("      Attr Name       : " + attrname.decode('ascii'))

    # ATTR PATH, 2 bytes length + string without \0
    sizeLimit = expectedAttrBlockLength - (f.tell() - startPosition)
    attrpath = ReadEncodedString(f, "Attr Path", sizeLimit)
    if (attrname == ""):
        return False
    print("      Attr Path       : " + attrpath.decode('ascii'))

    # 1 byte TYPE
    typeID = np.fromfile(f, dtype=np.uint8, count=1)[0]
    print("      Type            : {0} ({1}) ".format(
        bp4dbg_utils.GetTypeName(typeID), typeID))

    # End TAG AMD]
    tag = f.read(4)
    if (tag != b"AMD]"):
        print("  Tag: " + str(tag))
        print("ERROR: PG group metadata does not end with AMD]")
        return False
    print("      Tag             : {0}".format(tag.decode('ascii')))


# Read one PG process group (variables and attributes from one process in
# one step)
def ReadPG(f, fileSize, pgidx):
    pgStartPosition = f.tell()
    print("PG {0}: ".format(pgidx))
    print("  Starting offset : {0}".format(pgStartPosition))
    tag = f.read(4)
    if (tag != b"[PGI"):
        print("  Tag: " + str(tag))
        print("ERROR: PG group does not start with [PGI")
        return False

    print("  Tag             : " + tag.decode('ascii'))

    # 8 bytes PG Length
    pglen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("  PG length       : {0} bytes (+4 for Tag)".format(pglen))
    expectedPGLength = pglen + 4  # total length does not include opening tag
    if (pgStartPosition + expectedPGLength > fileSize):
        print("ERROR: There is not enough bytes in file to read this PG")
        return False

    # RowMajor 1 byte, 'y' or 'n'
    isRowMajor = f.read(1)
    if (isRowMajor != b'y' and isRowMajor != b'n'):
        print(
            "ERROR: Next byte for isRowMajor must be 'y' or 'n' "
            "but it isn't = {0}".format(isRowMajor))
        return False
    print("  isRowMajor      : " + isRowMajor.decode('ascii'))

    # PG Name, 2 bytes length + string without \0
    sizeLimit = expectedPGLength - (f.tell() - pgStartPosition)
    pgname = ReadEncodedString(
        f, "PG Name", sizeLimit)
    if (pgname == ""):
        return False
    print("  PG Name         : " + pgname.decode('ascii'))

    # 4 bytes unused (for Coordination variable)
    tag = f.read(4)
    print("  Unused 4 bytes  : " + str(tag))

    # Timestep name
    sizeLimit = expectedPGLength - (f.tell() - pgStartPosition)
    tsname = ReadEncodedString(f, "Timestep Name", sizeLimit)
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
        print("      Method ID   : {0}".format(methodID))
        sizeLimit = expectedPGLength - (f.tell() - pgStartPosition)
        methodParams = ReadEncodedString(
            f, "Method Parameters", sizeLimit)
        if (methodParams == ""):
            return False
        print('      M. params   : "' +
              methodParams.decode('ascii') + '"')

    # VARIABLES

    # VARS COUNT 4 bytes
    nVars = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("  # of Variables  : {0}".format(nVars))

    # VARS SIZE 8 bytes
    varlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("  Vars length     : {0} bytes".format(varlen))
    sizeLimit = expectedPGLength - (f.tell() - pgStartPosition)
    expectedVarsLength = varlen  # need to read this more
    if (expectedVarsLength > sizeLimit):
        print("ERROR: There is not enough bytes in PG to read the variables")
        return False

    varsStartPosition = f.tell()
    for i in range(nVars):
        # VMD block
        status = ReadVMD(f, i, varsStartPosition, expectedVarsLength)
        if (not status):
            return False

    # ATTRIBUTES

    # ATTRS COUNT 4 bytes
    nAttrs = np.fromfile(f, dtype=np.uint32, count=1)[0]
    print("  # of Attributes : {0}".format(nAttrs))

    # ATTS SIZE 8 bytes
    attlen = np.fromfile(f, dtype=np.uint64, count=1)[0]
    print("  Attrs length    : {0} bytes".format(attlen))
    sizeLimit = expectedPGLength - (f.tell() - pgStartPosition)
    expectedAttrsLength = attlen  # need to read this more
    if (expectedAttrsLength > sizeLimit):
        print("ERROR: There is not enough bytes in PG to read the attributes")
        return False

    attrsStartPosition = f.tell()
    for i in range(nAttrs):
        # AMD block
        status = ReadAMD(f, i, attrsStartPosition, expectedAttrsLength)
        if (not status):
            return False

    # End TAG PGI]
    tag = f.read(4)
    if (tag != b"PGI]"):
        print("  Tag: " + str(tag))
        print("ERROR: PG group metadata does not end with PGI]")
        return False
    print("  Tag               : {0}".format(tag.decode('ascii')))

    return True


def DumpData(fileName):
    print("=== Data File: " + fileName + " ====")
    with open(fileName, "rb") as f:
        fileSize = fstat(f.fileno()).st_size
        status = True
        pgidx = 0
        while (f.tell() < fileSize - 12 and status):
            status = ReadPG(f, fileSize, pgidx)
            pgidx = pgidx + 1


if __name__ == "__main__":
    print("ERROR: Utility main program is bp4dbg.py")
