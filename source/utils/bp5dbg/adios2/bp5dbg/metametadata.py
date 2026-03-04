from os import fstat

import numpy as np


def ReadMetaMetadataRecord(buf, rec, pos, fileSize):
    # Read one metametadata record
    startoffset = str(pos).rjust(8)
    # 8 bytes MetaMetaIDLen + MetaMetaInfoLen Length
    mmIDlen = np.frombuffer(buf, dtype=np.uint64, count=1, offset=pos)
    pos = pos + 8
    mmInfolen = np.frombuffer(buf, dtype=np.uint64, count=1, offset=pos)
    pos = pos + 8

    id_start = pos
    id_bytes = bytes(buf[id_start:id_start + int(mmIDlen[0])])
    pos = pos + int(mmIDlen[0])

    info_start = pos
    info_bytes = bytes(buf[info_start:info_start + int(mmInfolen[0])])
    pos = pos + int(mmInfolen[0])

    recs = str(rec).rjust(7)
    idlen = str(mmIDlen[0]).rjust(10)
    infolen = str(mmInfolen[0]).rjust(12)

    print(
        f" | {recs} | {startoffset} | {idlen} | {infolen} |")

    return pos, id_bytes, info_bytes


def DumpMetaMetaData(fileName, ffsDecoder=None):
    print("========================================================")
    print("    MetaMetadata File: " + fileName)
    print("========================================================")
    mmd_records = []
    with open(fileName, "rb") as f:
        fileSize = fstat(f.fileno()).st_size
        print(f" File size = {fileSize}")
        buf = f.read(fileSize)
        print(" --------------------------------------------------")
        print(" |  Record |  Offset  |  ID length |  Info length |")
        print(" --------------------------------------------------")
        pos = 0
        rec = 0
        while (pos < fileSize - 1):
            pos, id_bytes, info_bytes = ReadMetaMetadataRecord(
                buf, rec, pos, fileSize)
            mmd_records.append((id_bytes, info_bytes))
            rec = rec + 1
        print(" --------------------------------------------------")

        if ffsDecoder:
            _print_metametadata_fields(mmd_records, ffsDecoder)

    return True, mmd_records


def _print_metametadata_fields(mmd_records, ffsDecoder):
    """Print the decoded field lists from metametadata records."""
    ffsDecoder.load_metametadata(mmd_records)
    print()
    print("  Decoded format definitions:")
    for idx, (id_bytes, info_bytes) in enumerate(mmd_records):
        # Load this single record into a temporary context to get its
        # format info. We already loaded all records above, so we can
        # use FMformat_from_ID.
        fm_format = ffsDecoder.lib.FMformat_from_ID(
            ffsDecoder.fm_context, id_bytes)
        if not fm_format:
            continue
        format_name = ffsDecoder.lib.name_of_FMformat(fm_format)
        if format_name:
            format_name = format_name.decode('utf-8', errors='replace')
        else:
            format_name = f"<record {idx}>"

        desc_list_ptr = ffsDecoder.lib.format_list_of_FMFormat(fm_format)
        field_list = ffsDecoder._read_field_list(desc_list_ptr)

        print(f"  Format \"{format_name}\":")
        for name, ftype, fsize, foffset in field_list:
            print(f"    {name:40s} {ftype:30s} "
                  f"size={fsize} offset={foffset}")
        print()


if __name__ == "__main__":
    print("ERROR: Utility main program is bp5dbg.py")
