# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

from os import fstat
import ctypes
import struct

import numpy as np

from .utils import GetTypeName

# ADIOS2 DataType enum values (matching ADIOSTypes.h)
_adios_type_names = {
    1: "int8_t", 2: "int16_t", 3: "int32_t", 4: "int64_t",
    5: "uint8_t", 6: "uint16_t", 7: "uint32_t", 8: "uint64_t",
    9: "float", 10: "double", 11: "long double",
    12: "float complex", 13: "double complex",
    14: "string", 15: "char",
}

_shape_names = {
    'g': "GlobalValue",
    'G': "GlobalArray",
    'J': "JoinedArray",
    'l': "LocalValue",
    'L': "LocalArray",
}

_ptr_size = ctypes.sizeof(ctypes.c_void_p)
_size_t_size = ctypes.sizeof(ctypes.c_size_t)


def _ffs_type_to_adios_type(ffs_type, ffs_size):
    """Map FFS field type string to ADIOS2 DataType enum value."""
    if ffs_type == "string":
        return 14
    if ffs_type == "integer":
        return {1: 1, 2: 2, 4: 3, 8: 4}.get(ffs_size, 3)
    if ffs_type == "unsigned integer":
        return {1: 5, 2: 6, 4: 7, 8: 8}.get(ffs_size, 7)
    if ffs_type == "float":
        return {4: 9, 8: 10, 16: 11}.get(ffs_size, 10)
    if ffs_type == "complex":
        return {8: 12, 16: 13}.get(ffs_size, 12)
    if ffs_type == "char":
        return 15
    return 0


def _read_size_t(buf, offset):
    """Read a size_t from a ctypes buffer."""
    return ctypes.c_size_t.from_buffer_copy(buf, offset).value


def _read_ptr(buf, offset):
    """Read a pointer from a ctypes buffer."""
    return ctypes.c_void_p.from_buffer_copy(buf, offset).value


def _read_size_t_array(ptr, count):
    """Read an array of size_t values from a native pointer."""
    if not ptr or count == 0:
        return []
    arr_type = ctypes.c_size_t * count
    return list(arr_type.from_address(ptr))


def _read_string_ptr(buf, offset):
    """Read a char* (string pointer) from decoded buffer."""
    ptr = _read_ptr(buf, offset)
    if not ptr:
        return None
    return ctypes.string_at(ptr).decode('utf-8', errors='replace')


def _parse_field_name(field_name):
    """Parse a BP5 metadata field name.

    Returns (var_name, shape_char, elem_size, type_id, is_derived) or None.
    Field name format:
      BP<shape>_<elemsize>_<typeid>_<varname>  (when elemsize > 0)
      BP<shape>_<varname>                       (when elemsize == 0, e.g. strings)
      BP<shape>-<expression encoding>_<elemsize>_<typeid>_<varname>  (derived)
    """
    if not field_name.startswith("BP"):
        return None
    if len(field_name) < 4:
        return None

    shape_char = field_name[2]
    if shape_char not in ('g', 'G', 'J', 'l', 'L'):
        return None

    sep = field_name[3]
    is_derived = (sep == '-')

    if sep == '_':
        rest = field_name[4:]
    elif sep == '-':
        # Derived: expression encoded between dashes
        # Find the underscore after the expression
        # Format: BP<shape>-<len>-<base64>_<elemsize>_<typeid>_<name>
        dash_rest = field_name[4:]
        # Skip past the expression: find digits, dash, base64, then underscore
        import re
        m = re.match(r'(\d+)-([A-Za-z0-9+/=]+)-', dash_rest)
        if m:
            rest = dash_rest[m.end():]
        else:
            rest = dash_rest
    else:
        return None

    # Try to parse as <elemsize>_<typeid>_<varname>
    parts = rest.split('_', 2)
    if len(parts) >= 3:
        try:
            elem_size = int(parts[0])
            type_id = int(parts[1])
            var_name = parts[2]
            return var_name, shape_char, elem_size, type_id, is_derived
        except ValueError:
            pass

    # Fallback: no elem_size/type prefix (element_size == 0)
    # This happens for strings and V1-style scalar fields like BPg_nproc
    # Type will be determined from FFS field_type later
    return rest, shape_char, 0, 0, is_derived  # type 0 = unknown/from-ffs


def _type_name(type_id):
    """Get human-readable type name from ADIOS2 DataType enum."""
    return _adios_type_names.get(type_id, f"type({type_id})")


def _decode_scalar(decoded_buf, field_offset, field_size, type_id):
    """Read a scalar value from decoded buffer at given offset."""
    raw = bytes(decoded_buf[field_offset:field_offset + field_size])
    if len(raw) < field_size:
        return "<truncated>"

    fmt_map = {
        1: ('b', 1), 2: ('<h', 2), 3: ('<i', 4), 4: ('<q', 8),  # signed
        5: ('B', 1), 6: ('<H', 2), 7: ('<I', 4), 8: ('<Q', 8),  # unsigned
        9: ('<f', 4), 10: ('<d', 8),  # float/double
        15: ('c', 1),  # char
    }

    if type_id == 14:  # string
        ptr = _read_ptr(decoded_buf, field_offset)
        if ptr:
            return '"' + ctypes.string_at(ptr).decode(
                'utf-8', errors='replace') + '"'
        return '""'

    if type_id in fmt_map:
        fmt, sz = fmt_map[type_id]
        if type_id == 15:
            return repr(struct.unpack(fmt, raw[:sz])[0])
        return str(struct.unpack(fmt, raw[:sz])[0])

    if type_id == 11:  # long double
        return f"<long double, {field_size} bytes>"
    if type_id == 12:  # float complex
        r, i = struct.unpack('<ff', raw[:8])
        return f"({r}+{i}j)"
    if type_id == 13:  # double complex
        r, i = struct.unpack('<dd', raw[:16])
        return f"({r}+{i}j)"

    return f"<raw {field_size} bytes>"


def _decode_minmax(ptr, block_count, elem_size, type_id):
    """Decode MinMax array: 2 values (min, max) per block."""
    if not ptr or block_count == 0:
        return []

    total_bytes = 2 * block_count * elem_size
    raw = (ctypes.c_char * total_bytes).from_address(ptr)

    fmt_map = {
        1: 'b', 2: '<h', 3: '<i', 4: '<q',
        5: 'B', 6: '<H', 7: '<I', 8: '<Q',
        9: '<f', 10: '<d',
    }
    fmt = fmt_map.get(type_id)
    if not fmt:
        return []

    results = []
    for b in range(block_count):
        offset = 2 * b * elem_size
        mn = struct.unpack_from(fmt, raw, offset)[0]
        mx = struct.unpack_from(fmt, raw, offset + elem_size)[0]
        results.append((mn, mx))
    return results


def _print_array_field(decoded_buf, field_offset, field_type,
                       var_name, shape_char, elem_size, type_id, indent):
    """Print decoded array metadata from a MetaArrayRec variant."""
    # Determine variant from field_type
    has_operator = "Op" in field_type
    has_minmax = "MM" in field_type

    # MetaArrayRec layout (all size_t or pointers):
    # Dims (size_t), BlockCount (size_t), DBCount (size_t),
    # Shape (size_t*), Count (size_t*), Offsets (size_t*),
    # DataBlockLocation (size_t*)
    # Optional: DataBlockSize (size_t*) for Operator
    # Optional: MinMax (char*) for MM
    off = field_offset
    dims = _read_size_t(decoded_buf, off)
    off += _size_t_size
    block_count = _read_size_t(decoded_buf, off)
    off += _size_t_size
    db_count = _read_size_t(decoded_buf, off)
    off += _size_t_size

    shape_ptr = _read_ptr(decoded_buf, off)
    off += _ptr_size
    count_ptr = _read_ptr(decoded_buf, off)
    off += _ptr_size
    offsets_ptr = _read_ptr(decoded_buf, off)
    off += _ptr_size
    dataloc_ptr = _read_ptr(decoded_buf, off)
    off += _ptr_size

    datasize_ptr = None
    if has_operator:
        datasize_ptr = _read_ptr(decoded_buf, off)
        off += _ptr_size

    minmax_ptr = None
    if has_minmax:
        minmax_ptr = _read_ptr(decoded_buf, off)

    shape_name = _shape_names.get(shape_char, "Unknown")
    tname = _type_name(type_id)
    print(f"{indent}Var \"{var_name}\" ({shape_name}, {tname}, {dims}D)")
    print(f"{indent}  BlockCount: {block_count}")

    shape = _read_size_t_array(shape_ptr, dims)
    if shape:
        print(f"{indent}  Shape:  {shape}")

    counts = _read_size_t_array(count_ptr, db_count)
    offsets = _read_size_t_array(offsets_ptr, db_count)
    datalocs = _read_size_t_array(dataloc_ptr, block_count)
    datasizes = _read_size_t_array(datasize_ptr, block_count) \
        if has_operator else []

    minmax_vals = _decode_minmax(
        minmax_ptr, block_count, elem_size, type_id) \
        if has_minmax else []

    for b in range(block_count):
        blk_count = counts[b * dims:(b + 1) * dims] if counts else []
        blk_offset = offsets[b * dims:(b + 1) * dims] if offsets else []
        loc = datalocs[b] if b < len(datalocs) else "?"
        parts = [f"Count {blk_count}"]
        if blk_offset:
            parts.append(f"Offset {blk_offset}")
        parts.append(f"DataLoc {loc}")
        if has_operator and b < len(datasizes):
            parts.append(f"DataSize {datasizes[b]}")
        line = f"{indent}  Block {b}: " + " ".join(parts)
        print(line)
        if has_minmax and b < len(minmax_vals):
            mn, mx = minmax_vals[b]
            print(f"{indent}    Min: {mn}  Max: {mx}")


def _print_decoded_metadata(decoded_buf, field_list, kind, indent):
    """Print decoded variable or attribute metadata.

    Args:
        decoded_buf: ctypes buffer with decoded data
        field_list: list of (name, type, size, offset) from FFSDecoder
        kind: "variable" or "attribute"
        indent: indentation string
    """
    i = 0
    while i < len(field_list):
        name, ftype, fsize, foffset = field_list[i]

        # Skip BitField and DataBlockSize header fields
        if name.startswith("BitField") or name == "DataBlockSize":
            i += 1
            continue

        parsed = _parse_field_name(name)
        if not parsed:
            # Could be an attribute field (BP5_ prefix for V1 attrs)
            if kind == "attribute":
                _print_attr_field(decoded_buf, field_list, i, indent)
                # Attr arrays have two consecutive fields
                if name.endswith("ElemCount"):
                    i += 2
                else:
                    i += 1
            else:
                i += 1
            continue

        var_name, shape_char, elem_size, type_id, is_derived = parsed
        is_array = shape_char in ('G', 'L', 'J')

        if is_array:
            _print_array_field(decoded_buf, foffset, ftype,
                               var_name, shape_char, elem_size,
                               type_id, indent)
        else:
            # Scalar (GlobalValue or LocalValue)
            shape_name = _shape_names.get(shape_char, "Unknown")
            actual_type_id = type_id
            if actual_type_id == 0:
                # Infer from FFS field type
                actual_type_id = _ffs_type_to_adios_type(ftype, fsize)
            tname = _type_name(actual_type_id)
            val = _decode_scalar(decoded_buf, foffset, fsize, actual_type_id)
            print(f"{indent}Var \"{var_name}\" ({shape_name}, {tname})")
            print(f"{indent}  Value: {val}")

        i += 1


def _print_attr_field(decoded_buf, field_list, idx, indent):
    """Print a single attribute field from V1 attribute format."""
    name, ftype, fsize, foffset = field_list[idx]

    if name.endswith("ElemCount"):
        # Array attribute: ElemCount field followed by data field
        elem_count = _read_size_t(decoded_buf, foffset)
        if idx + 1 < len(field_list):
            data_name = field_list[idx + 1][0]
            # Attr name: strip "BP5_" prefix
            attr_name = data_name[4:] if data_name.startswith("BP5_") else \
                data_name
            print(f"{indent}Attr \"{attr_name}\" "
                  f"(array, {elem_count} elements)")
    elif name.startswith("BP5_") or name.startswith("bp5_"):
        # Simple scalar attribute
        attr_name = name[4:]
        # Try to figure out type from field_type
        val = _decode_attr_value(decoded_buf, foffset, ftype, fsize)
        print(f"{indent}Attr \"{attr_name}\" = {val}")


def _decode_attr_value(decoded_buf, offset, ftype, fsize):
    """Decode an attribute value based on its FFS field type."""
    if ftype == "string":
        return _read_string_ptr(decoded_buf, offset) or '""'
    if ftype == "integer":
        if fsize == 4:
            return struct.unpack_from('<i', decoded_buf, offset)[0]
        elif fsize == 8:
            return struct.unpack_from('<q', decoded_buf, offset)[0]
        elif fsize == 2:
            return struct.unpack_from('<h', decoded_buf, offset)[0]
        elif fsize == 1:
            return struct.unpack_from('<b', decoded_buf, offset)[0]
    if ftype == "unsigned integer":
        if fsize == 4:
            return struct.unpack_from('<I', decoded_buf, offset)[0]
        elif fsize == 8:
            return struct.unpack_from('<Q', decoded_buf, offset)[0]
    if ftype == "float":
        if fsize == 4:
            return struct.unpack_from('<f', decoded_buf, offset)[0]
        elif fsize == 8:
            return struct.unpack_from('<d', decoded_buf, offset)[0]
    return f"<{ftype}, {fsize} bytes>"


def _decode_step_metadata(f, buf, pos, WriterCount, mdpos, ffsDecoder):
    """Decode per-writer metadata blocks using FFS."""
    # pos is currently past the metadata size field.
    # The buffer layout is:
    #   [8 bytes mdsize_in_file]
    #   [WriterCount * 8 bytes: var md sizes]
    #   [WriterCount * 8 bytes: attr md sizes]
    #   [writer 0 var md] [writer 1 var md] ...
    #   [writer 0 attr md] [writer 1 attr md] ...

    # Read var md sizes
    var_sizes = []
    p = pos
    for w in range(WriterCount):
        a = np.frombuffer(buf, dtype=np.uint64, count=1, offset=p)
        var_sizes.append(int(a[0]))
        p += 8

    # Read attr md sizes
    attr_sizes = []
    for w in range(WriterCount):
        a = np.frombuffer(buf, dtype=np.uint64, count=1, offset=p)
        attr_sizes.append(int(a[0]))
        p += 8

    # Data starts right after the header
    data_offset = 8 + 2 * WriterCount * 8  # mdsize + var_sizes + attr_sizes

    # Decode variable metadata for each writer
    md_offset = data_offset
    for w in range(WriterCount):
        if var_sizes[w] > 0:
            encoded = buf[md_offset:md_offset + var_sizes[w]]
            result = ffsDecoder.decode(encoded)
            if result:
                field_list, decoded_data, format_name, fm_format = result
                print(f"  Writer {w} variable metadata (decoded):")
                _print_decoded_metadata(
                    decoded_data, field_list, "variable", "    ")
        md_offset += var_sizes[w]

    # Decode attribute metadata for each writer
    for w in range(WriterCount):
        if attr_sizes[w] > 0:
            encoded = buf[md_offset:md_offset + attr_sizes[w]]
            result = ffsDecoder.decode(encoded)
            if result:
                field_list, decoded_data, format_name, fm_format = result
                print(f"  Writer {w} attribute metadata (decoded):")
                if format_name == "GenericAttributes":
                    _print_generic_attrs(decoded_data, field_list, "    ")
                else:
                    _print_decoded_metadata(
                        decoded_data, field_list, "attribute", "    ")
        md_offset += attr_sizes[w]


def _print_generic_attrs(decoded_buf, field_list, indent):
    """Print GenericAttributes (V2 format) from decoded buffer.

    The V2 attribute format uses BP5AttrStruct with:
    - PrimAttrCount (size_t)
    - PrimAttrs (PrimitiveTypeAttr*) with Name, TotalElementSize, Values
    - StrAttrCount (size_t)
    - StrAttrs (StringArrayAttr*) with Name, ElementCount, Values
    """
    # Find fields in the decoded struct
    field_map = {f[0]: (f[1], f[2], f[3]) for f in field_list}

    if "PrimAttrCount" in field_map:
        _, _, off = field_map["PrimAttrCount"]
        prim_count = _read_size_t(decoded_buf, off)
        if prim_count > 0:
            print(f"{indent}Primitive attributes ({prim_count}):")
            # PrimAttrs is a pointer to array of PrimitiveTypeAttr structs
            if "PrimAttrs" in field_map:
                _, _, poff = field_map["PrimAttrs"]
                arr_ptr = _read_ptr(decoded_buf, poff)
                if arr_ptr:
                    _print_prim_attrs(arr_ptr, prim_count, indent + "  ")

    if "StrAttrCount" in field_map:
        _, _, off = field_map["StrAttrCount"]
        str_count = _read_size_t(decoded_buf, off)
        if str_count > 0:
            print(f"{indent}String attributes ({str_count}):")
            if "StrAttrs" in field_map:
                _, _, soff = field_map["StrAttrs"]
                arr_ptr = _read_ptr(decoded_buf, soff)
                if arr_ptr:
                    _print_str_attrs(arr_ptr, str_count, indent + "  ")


def _decode_attr_name(raw_name):
    """Decode V2 attribute name.

    First char encodes type: chr(ord('0') + DataType enum).
    If first char value >= ord('0')+18, it's an array attr.
    Actual name starts at index 1.
    """
    if not raw_name or len(raw_name) < 2:
        return raw_name, 0, False
    type_char = ord(raw_name[0])
    base_type = type_char - ord('0')
    is_array = False
    if base_type >= 18:
        is_array = True
        base_type -= 18
    return raw_name[1:], base_type, is_array


def _print_prim_attrs(arr_ptr, count, indent):
    """Print PrimitiveTypeAttr array."""
    # PrimitiveTypeAttr: Name (char*), TotalElementSize (size_t), Values (char*)
    rec_size = _ptr_size + _size_t_size + _ptr_size
    for i in range(count):
        base = arr_ptr + i * rec_size
        name_ptr = ctypes.c_void_p.from_address(base).value
        total_size = ctypes.c_size_t.from_address(
            base + _ptr_size).value
        values_ptr = ctypes.c_void_p.from_address(
            base + _ptr_size + _size_t_size).value
        raw_name = ""
        if name_ptr:
            raw_name = ctypes.string_at(name_ptr).decode(
                'utf-8', errors='replace')
        attr_name, type_id, is_array = _decode_attr_name(raw_name)
        tname = _type_name(type_id)
        elem_size = total_size
        if is_array and elem_size > 0:
            # TotalElementSize is total bytes for array attrs
            type_sz = {1: 1, 2: 2, 3: 4, 4: 8, 5: 1, 6: 2, 7: 4, 8: 8,
                       9: 4, 10: 8, 12: 8, 13: 16}.get(type_id, 0)
            n_elems = total_size // type_sz if type_sz else 0
            val_str = _read_prim_values(values_ptr, type_id, n_elems)
            print(f"{indent}Attr \"{attr_name}\" ({tname}[{n_elems}])"
                  f" = {val_str}")
        else:
            val_str = _read_prim_values(values_ptr, type_id, 1)
            print(f"{indent}Attr \"{attr_name}\" ({tname})"
                  f" = {val_str}")


def _read_prim_values(values_ptr, type_id, n_elems):
    """Read primitive attribute values from a pointer."""
    if not values_ptr or n_elems == 0:
        return "N/A"

    fmt_map = {
        1: ('b', 1), 2: ('h', 2), 3: ('i', 4), 4: ('q', 8),
        5: ('B', 1), 6: ('H', 2), 7: ('I', 4), 8: ('Q', 8),
        9: ('f', 4), 10: ('d', 8),
    }
    if type_id not in fmt_map:
        return f"<type {type_id}>"

    fmt, sz = fmt_map[type_id]
    total = n_elems * sz
    raw = (ctypes.c_char * total).from_address(values_ptr)
    values = []
    for j in range(n_elems):
        val = struct.unpack_from('<' + fmt, raw, j * sz)[0]
        values.append(val)
    if n_elems == 1:
        return str(values[0])
    return str(values)


def _print_str_attrs(arr_ptr, count, indent):
    """Print StringArrayAttr array."""
    # StringArrayAttr: Name (char*), ElementCount (size_t), Values (char**)
    rec_size = _ptr_size + _size_t_size + _ptr_size
    for i in range(count):
        base = arr_ptr + i * rec_size
        name_ptr = ctypes.c_void_p.from_address(base).value
        elem_count = ctypes.c_size_t.from_address(
            base + _ptr_size).value
        values_ptr = ctypes.c_void_p.from_address(
            base + _ptr_size + _size_t_size).value
        raw_name = ""
        if name_ptr:
            raw_name = ctypes.string_at(name_ptr).decode(
                'utf-8', errors='replace')
        attr_name, type_id, is_array = _decode_attr_name(raw_name)
        if elem_count == 1 and values_ptr:
            str_ptr = ctypes.c_void_p.from_address(values_ptr).value
            if str_ptr:
                val = ctypes.string_at(str_ptr).decode(
                    'utf-8', errors='replace')
                print(f"{indent}Attr \"{attr_name}\" (string) = \"{val}\"")
            else:
                print(f"{indent}Attr \"{attr_name}\" (string) = \"\"")
        elif elem_count > 1 and values_ptr:
            strs = []
            for j in range(elem_count):
                sp = ctypes.c_void_p.from_address(
                    values_ptr + j * _ptr_size).value
                if sp:
                    strs.append(ctypes.string_at(sp).decode(
                        'utf-8', errors='replace'))
                else:
                    strs.append("")
            print(f"{indent}Attr \"{attr_name}\" (string[{elem_count}])"
                  f" = {strs}")
        else:
            print(f"{indent}Attr \"{attr_name}\" (string) = \"\"")


def ReadMetadataStep(f, fileSize, MetadataEntry, WriterMapEntry,
                     ffsDecoder=None):
    # Read metadata of one step
    step = MetadataEntry['step']
    mdpos = MetadataEntry['mdpos']
    mdsize = MetadataEntry['mdsize']
    # flushcount = MetadataEntry['flushcount']
    WriterCount = WriterMapEntry['WriterCount']

    if mdpos + mdsize > fileSize:
        print(f"ERROR: step {step} metadata pos {mdpos} + size {mdsize} "
              f"is beyond the metadata file size {fileSize}")
        return False

    currentpos = f.tell()
    if mdpos > currentpos:
        print(f"Offset {currentpos}..{mdpos-1} is a gap unaccounted for")

    if mdpos < currentpos:
        print(f"ERROR: step {step} metadata pos {mdpos} points before the "
              f"expected position in file {currentpos}")
        return False

    f.seek(mdpos)
    buf = f.read(mdsize)
    pos = 0

    if step > 0:
        print("========================================================")
    print(f"Step {step}: ")
    print(f"  Offset = {mdpos}")

    mdsize_in_file = np.frombuffer(buf, dtype=np.uint64, count=1,
                                   offset=pos)
    pos = pos + 8
    if (mdsize == mdsize_in_file[0] + 8):
        print(f"  Size = {mdsize_in_file[0]}")
    else:
        print(f"ERROR: md record supposed to be {mdsize-8} + 8 bytes "
              f"(as recorded in index), but found in file "
              f"{mdsize_in_file[0]}")

    MDPosition = mdpos + 2 * 8 * WriterCount
    print("  Variable metadata entries: ")
    for w in range(0, WriterCount):
        a = np.frombuffer(buf, dtype=np.uint64, count=1, offset=pos)
        thisMDSize = int(a[0])
        pos = pos + 8
        print(f"    Writer {w}: md size {thisMDSize} "
              f"offset {MDPosition}")
        MDPosition = MDPosition + thisMDSize

    print("  Attribute metadata entries: ")
    for w in range(0, WriterCount):
        a = np.frombuffer(buf, dtype=np.uint64, count=1, offset=pos)
        thisMDSize = int(a[0])
        pos = pos + 8
        print(f"    Writer {w}: md size {thisMDSize} "
              f"offset {MDPosition}")
        MDPosition = MDPosition + thisMDSize

    if (mdsize_in_file != MDPosition - mdpos):
        print(f"ERROR: entries supposed to end at start offset+size "
              f"{mdpos}+{mdsize_in_file[0]}, but it ends instead on offset "
              f"{MDPosition}")

    # Decode with FFS if available
    if ffsDecoder:
        _decode_step_metadata(f, buf, 8, WriterCount, mdpos, ffsDecoder)

    return True


def DumpMetaData(fileName, MetadataIndexTable, WriterMap, ffsDecoder=None):
    print("========================================================")
    print("    Metadata File: " + fileName)
    print("========================================================")

    # print(f"MetadataIndexTable={MetadataIndexTable}")
    # print(f"WriterMap={WriterMap}")

    with open(fileName, "rb") as f:
        fileSize = fstat(f.fileno()).st_size
        for MetadataEntry in MetadataIndexTable:
            WriterMapEntry = WriterMap[MetadataEntry['writermapindex']]
            status = ReadMetadataStep(
                f, fileSize, MetadataEntry, WriterMapEntry, ffsDecoder)
    return status


if __name__ == "__main__":
    print("ERROR: Utility main program is bp5dbg.py")
