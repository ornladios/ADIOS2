# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""
ctypes wrapper for libadios2_ffs to decode FFS-encoded BP5 metadata.
"""

import ctypes
import ctypes.util
import os
import struct
import sys


class FFSDecoder:
    """Wraps libadios2_ffs C functions for decoding FFS-encoded metadata."""

    def __init__(self, lib_path=None):
        self.lib = self._load_library(lib_path)
        self._setup_prototypes()
        self.ffs_context = self.lib.create_FFSContext_FM(None)
        self.fm_context = self.lib.FMContext_from_FFS(self.ffs_context)

    @staticmethod
    def _load_library(lib_path=None):
        """Try to load libadios2_ffs from various locations."""
        paths_to_try = []

        if lib_path:
            paths_to_try.append(lib_path)

        env_path = os.environ.get("ADIOS2_FFS_LIB")
        if env_path:
            paths_to_try.append(env_path)

        # Sibling to this script: ../../lib/libadios2_ffs.{dylib,so}
        script_dir = os.path.dirname(os.path.abspath(__file__))
        for ext in ("dylib", "so"):
            paths_to_try.append(os.path.join(script_dir, "..", "..", "lib", f"libadios2_ffs.{ext}"))

        for path in paths_to_try:
            path = os.path.realpath(path)
            if os.path.isfile(path):
                try:
                    return ctypes.CDLL(path)
                except OSError:
                    continue

        # Fall back to system search
        name = ctypes.util.find_library("adios2_ffs")
        if name:
            try:
                return ctypes.CDLL(name)
            except OSError:
                pass

        raise OSError("Cannot load libadios2_ffs. Set ADIOS2_FFS_LIB or use --ffs-lib.")

    def _setup_prototypes(self):
        lib = self.lib

        # FFSContext create_FFSContext_FM(FMContext)
        lib.create_FFSContext_FM.restype = ctypes.c_void_p
        lib.create_FFSContext_FM.argtypes = [ctypes.c_void_p]

        # FMContext FMContext_from_FFS(FFSContext)
        lib.FMContext_from_FFS.restype = ctypes.c_void_p
        lib.FMContext_from_FFS.argtypes = [ctypes.c_void_p]

        # void free_FFSContext(FFSContext)
        lib.free_FFSContext.restype = None
        lib.free_FFSContext.argtypes = [ctypes.c_void_p]

        # FMFormat load_external_format_FMcontext(FMContext, char*, int, char*)
        lib.load_external_format_FMcontext.restype = ctypes.c_void_p
        lib.load_external_format_FMcontext.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_char_p,
        ]

        # FFSTypeHandle FFSTypeHandle_from_encode(FFSContext, char*)
        lib.FFSTypeHandle_from_encode.restype = ctypes.c_void_p
        lib.FFSTypeHandle_from_encode.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        # size_t FFS_est_decode_length(FFSContext, char*, size_t)
        lib.FFS_est_decode_length.restype = ctypes.c_size_t
        lib.FFS_est_decode_length.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]

        # int FFSdecode_to_buffer(FFSContext, char*, void*)
        lib.FFSdecode_to_buffer.restype = ctypes.c_int
        lib.FFSdecode_to_buffer.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]

        # FMFormat FMFormat_of_original(FFSTypeHandle)
        lib.FMFormat_of_original.restype = ctypes.c_void_p
        lib.FMFormat_of_original.argtypes = [ctypes.c_void_p]

        # FMStructDescList format_list_of_FMFormat(FMFormat)
        lib.format_list_of_FMFormat.restype = ctypes.c_void_p
        lib.format_list_of_FMFormat.argtypes = [ctypes.c_void_p]

        # char* name_of_FMformat(FMFormat)
        lib.name_of_FMformat.restype = ctypes.c_char_p
        lib.name_of_FMformat.argtypes = [ctypes.c_void_p]

        # int FMdump_data(FMFormat, void*, int)
        lib.FMdump_data.restype = ctypes.c_int
        lib.FMdump_data.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]

        # int FFShas_conversion(FFSTypeHandle)
        lib.FFShas_conversion.restype = ctypes.c_int
        lib.FFShas_conversion.argtypes = [ctypes.c_void_p]

        # void establish_conversion(FFSContext, FFSTypeHandle, FMStructDescList)
        lib.establish_conversion.restype = None
        lib.establish_conversion.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]

        # FMStructDescList FMcopy_struct_list(FMStructDescList)
        lib.FMcopy_struct_list.restype = ctypes.c_void_p
        lib.FMcopy_struct_list.argtypes = [ctypes.c_void_p]

        # void FMfree_struct_list(FMStructDescList)
        lib.FMfree_struct_list.restype = None
        lib.FMfree_struct_list.argtypes = [ctypes.c_void_p]

        # FMFormat FMformat_from_ID(FMContext, char*)
        lib.FMformat_from_ID.restype = ctypes.c_void_p
        lib.FMformat_from_ID.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        # int FFSdecode_in_place_possible(FFSTypeHandle)
        lib.FFSdecode_in_place_possible.restype = ctypes.c_int
        lib.FFSdecode_in_place_possible.argtypes = [ctypes.c_void_p]

        # int FFSdecode_in_place(FFSContext, char*, void**)
        lib.FFSdecode_in_place.restype = ctypes.c_int
        lib.FFSdecode_in_place.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]

    def load_metametadata(self, mmd_records):
        """Load metametadata records into the FFS context.

        Args:
            mmd_records: list of (id_bytes, info_bytes) tuples
        """
        for id_bytes, info_bytes in mmd_records:
            self.lib.load_external_format_FMcontext(
                self.fm_context, id_bytes, len(id_bytes), info_bytes
            )

    def decode(self, encoded_buf):
        """Decode an FFS-encoded metadata buffer.

        Returns:
            (field_list, decoded_data, format_name, fm_format) where:
            - field_list is a list of (name, type, size, offset) tuples
            - decoded_data is a ctypes buffer with the decoded data
            - format_name is the format name string
            - fm_format is the FMFormat pointer (for FMdump_data)
        """
        buf = ctypes.create_string_buffer(bytes(encoded_buf))

        ffs_type = self.lib.FFSTypeHandle_from_encode(self.ffs_context, buf)
        if not ffs_type:
            return None

        # Establish conversion if needed
        if not self.lib.FFShas_conversion(ffs_type):
            fm_format = self.lib.FMformat_from_ID(self.fm_context, buf)
            desc_list = self.lib.FMcopy_struct_list(self.lib.format_list_of_FMFormat(fm_format))
            self.lib.establish_conversion(self.ffs_context, ffs_type, desc_list)
            self.lib.FMfree_struct_list(desc_list)

        # Decode
        decode_len = self.lib.FFS_est_decode_length(self.ffs_context, buf, len(encoded_buf))
        decoded = ctypes.create_string_buffer(decode_len)
        self.lib.FFSdecode_to_buffer(self.ffs_context, buf, decoded)

        # Get format info
        fm_format = self.lib.FMFormat_of_original(ffs_type)
        format_name = self.lib.name_of_FMformat(fm_format)
        if format_name:
            format_name = format_name.decode("utf-8", errors="replace")

        # Extract field list
        desc_list_ptr = self.lib.format_list_of_FMFormat(fm_format)
        field_list = self._read_field_list(desc_list_ptr)

        return field_list, decoded, format_name, fm_format

    def dump_data(self, fm_format, decoded_data):
        """Call FMdump_data for verbose output."""
        self.lib.FMdump_data(fm_format, decoded_data, 1024000)

    def _read_field_list(self, desc_list_ptr):
        """Read the FMStructDescList[0].field_list into Python tuples.

        FMStructDescRec has: format_name (char*), field_list (FMFieldList),
                            struct_size (int), opt_info (ptr)
        FMField has: field_name (char*), field_type (char*),
                    field_size (int), field_offset (int)
        """
        if not desc_list_ptr:
            return []

        # FMStructDescRec: char*, FMFieldList(ptr), int, ptr
        # On 64-bit: 8 + 8 + 4 + padding + 8 = 32 bytes typically
        # Read the field_list pointer (second field in first struct)
        ptr_size = ctypes.sizeof(ctypes.c_void_p)
        # format_name is at offset 0 (ptr), field_list at offset ptr_size
        field_list_ptr = ctypes.c_void_p.from_address(desc_list_ptr + ptr_size).value
        if not field_list_ptr:
            return []

        # FMField: char* field_name, char* field_type, int field_size,
        #          int field_offset
        # Size: ptr + ptr + int + int = 8+8+4+4 = 24 on 64-bit
        field_struct_size = 2 * ptr_size + 2 * ctypes.sizeof(ctypes.c_int)

        fields = []
        i = 0
        while True:
            base = field_list_ptr + i * field_struct_size
            name_ptr = ctypes.c_void_p.from_address(base).value
            if not name_ptr:
                break
            type_ptr = ctypes.c_void_p.from_address(base + ptr_size).value

            name = ctypes.string_at(name_ptr).decode("utf-8", errors="replace")
            ftype = ctypes.string_at(type_ptr).decode("utf-8", errors="replace")
            fsize = ctypes.c_int.from_address(base + 2 * ptr_size).value
            foffset = ctypes.c_int.from_address(
                base + 2 * ptr_size + ctypes.sizeof(ctypes.c_int)
            ).value

            fields.append((name, ftype, fsize, foffset))
            i += 1

        return fields

    def close(self):
        """Free the FFS context."""
        if self.ffs_context:
            try:
                self.lib.free_FFSContext(self.ffs_context)
            except Exception:
                pass
            self.ffs_context = None
            self.fm_context = None
