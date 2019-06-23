from __future__ import absolute_import, division, print_function, unicode_literals

dataTypes = {
    -1: 'unknown',
    0: 'byte',
    1: 'short',
    2: 'integer',
    4: 'long',

    50: 'unsigned_byte',
    51: 'unsigned_short',
    52: 'unsigned_integer',
    54: 'unsigned_long',

    5: 'real',
    6: 'double',
    7: 'long_double',

    9: 'string',
    10: 'complex',
    11: 'double_complex',
    12: 'string_array'
}

dataTypeSize = {
    -1: 0,
    0: 1,
    1: 2,
    2: 4,
    4: 8,

    50: 1,
    51: 2,
    52: 4,
    54: 8,

    5: 4,
    6: 8,
    7: 16,

    9: 0,
    10: 8,
    11: 16,
    12: 0
}


def GetTypeName(typeID):
    name = dataTypes.get(typeID)
    if (name == None):
        name = "unknown type"
    return name


def GetTypeSize(typeID):
    size = dataTypeSize.get(typeID)
    if (size == None):
        size = 0
    return size


CharacteristicNames = {
    0: 'value',
    1: 'min',
    2: 'max',
    3: 'offset',
    4: 'dimensions',
    5: 'var_id',
    6: 'payload_offset',
    7: 'file_index',
    8: 'time_index',
    9: 'bitmap',
    10: 'stat',
    11: 'transform_type'
}


def GetCharacteristicName(cID):
    name = CharacteristicNames.get(cID)
    if (name == None):
        name = "unknown characteristic"
    return name


def GetCharacteristicDataLength(cID, typeID):
    name = CharacteristicNames.get(cID)
    if (name == 'value' or name == 'min' or name == 'max'):
        return dataTypeSize[typeID]
    elif (name == 'offset' or name == 'payload offset'):
        return 8
    elif (name == 'file_index' or name == 'time_index'):
        return 4
    else:
        return 0


if __name__ == "__main__":
    print("ERROR: Utility main program is bp4dbg.py")
