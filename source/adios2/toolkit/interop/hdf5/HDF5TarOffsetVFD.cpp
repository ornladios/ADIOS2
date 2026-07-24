/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "HDF5TarOffsetVFD.h"

#ifdef H5FD_CLASS_VERSION
#include <H5FDdevelop.h>
#else
#include <H5FDpublic.h>
#endif

#include <algorithm> // std::copy
#include <iterator>  // std::begin, std::end
#include <limits>    // optional, for overflow checking
#include <mutex>     // std::once_flag, std::call_once

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>  // open, O_RDONLY
#include <unistd.h> // close, pread, off_t, ssize_t
#endif

struct TarOffsetFAPL
{
    haddr_t base_offset; // ti.offset_data
    haddr_t member_size; // ti.size
};

struct TarOffsetFile
{
    H5FD_t pub; // must be first

#if defined(_WIN32)
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif

    haddr_t base_offset;
    haddr_t member_size;
    haddr_t eoa;
};

static void *taroffset_fapl_copy(const void *fapl)
{
    auto *src = static_cast<const TarOffsetFAPL *>(fapl);
    auto *dst = new TarOffsetFAPL(*src);
    return dst;
}

static herr_t taroffset_fapl_free(void *fapl)
{
    delete static_cast<TarOffsetFAPL *>(fapl);
    return 0;
}

static H5FD_t *taroffset_open(const char *name, unsigned flags, hid_t fapl_id,
                              haddr_t /* maxaddr */)
{
    if ((flags & H5F_ACC_RDWR) != 0)
    {
        return nullptr;
    }

    const auto *fa = static_cast<const TarOffsetFAPL *>(H5Pget_driver_info(fapl_id));

    if (fa == nullptr)
    {
        return nullptr;
    }

    auto *file = new TarOffsetFile{};

#ifdef _WIN32
    file->handle = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file->handle == INVALID_HANDLE_VALUE)
    {
        delete file;
        return nullptr;
    }
#else
    file->fd = ::open(name, O_RDONLY);

    if (file->fd < 0)
    {
        delete file;
        return nullptr;
    }
#endif

    file->base_offset = fa->base_offset;
    file->member_size = fa->member_size;
    file->eoa = fa->member_size;

    return reinterpret_cast<H5FD_t *>(file);
}

static herr_t taroffset_close(H5FD_t *f)
{
    auto *file = reinterpret_cast<TarOffsetFile *>(f);
    herr_t result = 0;

#ifdef _WIN32
    if (file->handle != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(file->handle))
        {
            result = -1;
        }
    }
#else
    if (file->fd >= 0)
    {
        if (::close(file->fd) < 0)
        {
            result = -1;
        }
    }
#endif

    delete file;
    return result;
}

static herr_t taroffset_read(H5FD_t *f, H5FD_mem_t /* type */, hid_t /* dxpl_id */, haddr_t addr,
                             size_t size, void *buffer)
{
    auto *file = reinterpret_cast<TarOffsetFile *>(f);

    if (addr > file->member_size || size > file->member_size - addr)
    {
        return -1;
    }

    if (size == 0)
    {
        return 0;
    }

    if (file->base_offset > HADDR_MAX - addr)
    {
        return -1;
    }

    haddr_t physical = file->base_offset + addr;
    auto *dst = static_cast<unsigned char *>(buffer);
    size_t remaining = size;

#ifdef _WIN32

    while (remaining > 0)
    {
        // ReadFile uses a DWORD count, so split larger requests.
        const DWORD request =
            remaining > static_cast<size_t>(MAXDWORD) ? MAXDWORD : static_cast<DWORD>(remaining);

        OVERLAPPED overlapped{};
        overlapped.Offset = static_cast<DWORD>(physical & 0xffffffffULL);
        overlapped.OffsetHigh = static_cast<DWORD>((physical >> 32) & 0xffffffffULL);

        DWORD bytesRead = 0;

        if (!ReadFile(file->handle, dst, request, &bytesRead, &overlapped))
        {
            return -1;
        }

        if (bytesRead == 0)
        {
            return -1;
        }

        dst += bytesRead;
        physical += bytesRead;
        remaining -= bytesRead;
    }

#else

    while (remaining > 0)
    {
        /*
         * Prevent a size_t-to-ssize_t conversion overflow.
         * POSIX read functions cannot return more than SSIZE_MAX.
         */
        const size_t request =
            std::min(remaining, static_cast<size_t>(std::numeric_limits<ssize_t>::max()));

        const auto physicalOffset = static_cast<off_t>(physical);

        /*
         * Detect failure if off_t cannot represent the HDF5 address.
         */
        if (physicalOffset < 0 || static_cast<haddr_t>(physicalOffset) != physical)
        {
            return -1;
        }

        const ssize_t n = ::pread(file->fd, dst, request, physicalOffset);

        if (n <= 0)
        {
            return -1;
        }

        dst += static_cast<size_t>(n);
        physical += static_cast<haddr_t>(n);
        remaining -= static_cast<size_t>(n);
    }

#endif

    return 0;
}

static herr_t taroffset_write(H5FD_t *, H5FD_mem_t, hid_t, haddr_t, size_t, const void *)
{
    return -1; // read-only
}

static haddr_t taroffset_get_eoa(const H5FD_t *f, H5FD_mem_t)
{
    const auto *file = reinterpret_cast<const TarOffsetFile *>(f);
    return file->eoa;
}

static herr_t taroffset_set_eoa(H5FD_t *f, H5FD_mem_t, haddr_t addr)
{
    auto *file = reinterpret_cast<TarOffsetFile *>(f);

    if (addr > file->member_size)
    {
        return -1;
    }

    file->eoa = addr;
    return 0;
}

static haddr_t taroffset_get_eof(const H5FD_t *f, H5FD_mem_t)
{
    const auto *file = reinterpret_cast<const TarOffsetFile *>(f);
    return file->member_size;
}

static herr_t taroffset_query(const H5FD_t *, unsigned long *flags)
{
    *flags = 0;
    return 0;
}

static herr_t taroffset_flush(H5FD_t *, hid_t, hbool_t) { return 0; }

static herr_t taroffset_truncate(H5FD_t *, hid_t, hbool_t) { return 0; }

static const H5FD_class_t *taroffset_get_class()
{
    static H5FD_class_t cls{};
    static std::once_flag once;

    std::call_once(once, [] {
#ifdef H5FD_CLASS_VERSION
        /*
         * HDF5 1.14 added these two fields at the beginning of
         * H5FD_class_t.
         */
        cls.version = H5FD_CLASS_VERSION;

        /*
         * Values 256-511 are reserved for experimental/custom drivers.
         * This value must not conflict with another VFD in the process.
         */
        cls.value = static_cast<H5FD_class_value_t>(501);
#endif

        cls.name = "taroffset";
        cls.maxaddr = HADDR_MAX;
        cls.fc_degree = H5F_CLOSE_WEAK;

        cls.terminate = nullptr;

        cls.sb_size = nullptr;
        cls.sb_encode = nullptr;
        cls.sb_decode = nullptr;

        cls.fapl_size = sizeof(TarOffsetFAPL);
        cls.fapl_get = nullptr;
        cls.fapl_copy = taroffset_fapl_copy;
        cls.fapl_free = taroffset_fapl_free;

        cls.dxpl_size = 0;
        cls.dxpl_copy = nullptr;
        cls.dxpl_free = nullptr;

        cls.open = taroffset_open;
        cls.close = taroffset_close;
        cls.cmp = nullptr;
        cls.query = taroffset_query;

        cls.get_type_map = nullptr;
        cls.alloc = nullptr;
        cls.free = nullptr;

        cls.get_eoa = taroffset_get_eoa;
        cls.set_eoa = taroffset_set_eoa;
        cls.get_eof = taroffset_get_eof;

        cls.get_handle = nullptr;

        cls.read = taroffset_read;
        cls.write = taroffset_write;

#ifdef H5FD_CLASS_VERSION
        /*
         * HDF5 falls back to scalar read/write callbacks when these
         * optional callbacks are null.
         */
        cls.read_vector = nullptr;
        cls.write_vector = nullptr;
        cls.read_selection = nullptr;
        cls.write_selection = nullptr;
#endif

        cls.flush = taroffset_flush;
        cls.truncate = taroffset_truncate;
        cls.lock = nullptr;
        cls.unlock = nullptr;

#ifdef H5FD_CLASS_VERSION
        cls.del = nullptr;
        cls.ctl = nullptr;
#endif

        const H5FD_mem_t map[H5FD_MEM_NTYPES] = H5FD_FLMAP_SINGLE;

        std::copy(std::begin(map), std::end(map), std::begin(cls.fl_map));
    });

    return &cls;
}

static hid_t register_taroffset_vfd()
{
    static hid_t driver_id = H5I_INVALID_HID;
    static std::once_flag once;

    std::call_once(once, [] { driver_id = H5FDregister(taroffset_get_class()); });

    return driver_id;
}

hid_t H5Pset_fapl_taroffset(haddr_t offset, haddr_t size)
{
    TarOffsetFAPL fa;
    hid_t driver_id = register_taroffset_vfd();
    if (driver_id < 0)
    {
        return H5I_INVALID_HID;
    }
    fa.base_offset = offset;
    fa.member_size = size;
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    if (fapl < 0)
    {
        return H5I_INVALID_HID;
    }
    if (H5Pset_driver(fapl, driver_id, &fa) < 0)
    {
        H5Pclose(fapl);
        return H5I_INVALID_HID;
    }
    return fapl;
}