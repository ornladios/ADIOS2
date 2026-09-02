/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HDF5TransportVFD.h"

#ifdef H5FD_CLASS_VERSION
#include <H5FDdevelop.h>
#else
#include <H5FDpublic.h>
#endif

#include "adios2/toolkit/transport/Transport.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>

namespace
{

struct TransportFAPL
{
    const std::shared_ptr<adios2::Transport> *transport;
    haddr_t size;
};

struct TransportFile
{
    H5FD_t pub; // must be first
    std::shared_ptr<adios2::Transport> transport;
    haddr_t size = 0;
    haddr_t eoa = 0;
    std::mutex mutex;
};

void *TransportFAPLCopy(const void *fapl)
{
    try
    {
        const auto *source = static_cast<const TransportFAPL *>(fapl);
        auto *transport = new std::shared_ptr<adios2::Transport>(*source->transport);
        try
        {
            return new TransportFAPL{transport, source->size};
        }
        catch (...)
        {
            delete transport;
            throw;
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

herr_t TransportFAPLFree(void *fapl)
{
    auto *properties = static_cast<TransportFAPL *>(fapl);
    delete properties->transport;
    delete properties;
    return 0;
}

H5FD_t *TransportOpen(const char *, unsigned flags, hid_t faplID, haddr_t)
{
    if ((flags & H5F_ACC_RDWR) != 0)
    {
        return nullptr;
    }

    const auto *properties = static_cast<const TransportFAPL *>(H5Pget_driver_info(faplID));
    if (properties == nullptr || properties->transport == nullptr || !*properties->transport)
    {
        return nullptr;
    }

    try
    {
        auto *file = new TransportFile{};
        file->transport = *properties->transport;
        file->size = properties->size;
        file->eoa = properties->size;
        return reinterpret_cast<H5FD_t *>(file);
    }
    catch (...)
    {
        return nullptr;
    }
}

herr_t TransportClose(H5FD_t *fileHandle)
{
    auto *file = reinterpret_cast<TransportFile *>(fileHandle);
    herr_t result = 0;
    try
    {
        if (file->transport && file->transport->m_IsOpen)
        {
            file->transport->Close();
        }
    }
    catch (...)
    {
        result = -1;
    }
    delete file;
    return result;
}

herr_t TransportRead(H5FD_t *fileHandle, H5FD_mem_t, hid_t, haddr_t address, size_t size,
                     void *buffer)
{
    auto *file = reinterpret_cast<TransportFile *>(fileHandle);
    if (address > file->size || size > file->size - address)
    {
        return -1;
    }
    if (size == 0)
    {
        return 0;
    }
    if (address > static_cast<haddr_t>(std::numeric_limits<size_t>::max()))
    {
        return -1;
    }

    try
    {
        std::lock_guard<std::mutex> lock(file->mutex);
        file->transport->Read(static_cast<char *>(buffer), size, static_cast<size_t>(address));
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

herr_t TransportWrite(H5FD_t *, H5FD_mem_t, hid_t, haddr_t, size_t, const void *) { return -1; }

haddr_t TransportGetEOA(const H5FD_t *fileHandle, H5FD_mem_t)
{
    return reinterpret_cast<const TransportFile *>(fileHandle)->eoa;
}

herr_t TransportSetEOA(H5FD_t *fileHandle, H5FD_mem_t, haddr_t address)
{
    auto *file = reinterpret_cast<TransportFile *>(fileHandle);
    if (address > file->size)
    {
        return -1;
    }
    file->eoa = address;
    return 0;
}

haddr_t TransportGetEOF(const H5FD_t *fileHandle, H5FD_mem_t)
{
    return reinterpret_cast<const TransportFile *>(fileHandle)->size;
}

herr_t TransportQuery(const H5FD_t *, unsigned long *flags)
{
    *flags = 0;
    return 0;
}

herr_t TransportFlush(H5FD_t *, hid_t, hbool_t) { return 0; }

herr_t TransportTruncate(H5FD_t *, hid_t, hbool_t) { return 0; }

const H5FD_class_t *GetTransportClass()
{
    static H5FD_class_t driverClass{};
    static std::once_flag once;

    std::call_once(once, [] {
#ifdef H5FD_CLASS_VERSION
        driverClass.version = H5FD_CLASS_VERSION;
        driverClass.value = static_cast<H5FD_class_value_t>(502);
#endif
        driverClass.name = "adios2_transport";
        driverClass.maxaddr = HADDR_MAX;
        driverClass.fc_degree = H5F_CLOSE_WEAK;
        driverClass.terminate = nullptr;
        driverClass.sb_size = nullptr;
        driverClass.sb_encode = nullptr;
        driverClass.sb_decode = nullptr;
        driverClass.fapl_size = sizeof(TransportFAPL);
        driverClass.fapl_get = nullptr;
        driverClass.fapl_copy = TransportFAPLCopy;
        driverClass.fapl_free = TransportFAPLFree;
        driverClass.dxpl_size = 0;
        driverClass.dxpl_copy = nullptr;
        driverClass.dxpl_free = nullptr;
        driverClass.open = TransportOpen;
        driverClass.close = TransportClose;
        driverClass.cmp = nullptr;
        driverClass.query = TransportQuery;
        driverClass.get_type_map = nullptr;
        driverClass.alloc = nullptr;
        driverClass.free = nullptr;
        driverClass.get_eoa = TransportGetEOA;
        driverClass.set_eoa = TransportSetEOA;
        driverClass.get_eof = TransportGetEOF;
        driverClass.get_handle = nullptr;
        driverClass.read = TransportRead;
        driverClass.write = TransportWrite;
#ifdef H5FD_CLASS_VERSION
        driverClass.read_vector = nullptr;
        driverClass.write_vector = nullptr;
        driverClass.read_selection = nullptr;
        driverClass.write_selection = nullptr;
#endif
        driverClass.flush = TransportFlush;
        driverClass.truncate = TransportTruncate;
        driverClass.lock = nullptr;
        driverClass.unlock = nullptr;
#ifdef H5FD_CLASS_VERSION
        driverClass.del = nullptr;
        driverClass.ctl = nullptr;
#endif

        const H5FD_mem_t map[H5FD_MEM_NTYPES] = H5FD_FLMAP_SINGLE;
        std::copy(std::begin(map), std::end(map), std::begin(driverClass.fl_map));
    });
    return &driverClass;
}

hid_t RegisterTransportVFD()
{
    static hid_t driverID = H5I_INVALID_HID;
    static std::once_flag once;
    std::call_once(once, [] { driverID = H5FDregister(GetTransportClass()); });
    return driverID;
}

} // end anonymous namespace

hid_t H5Pset_fapl_adios2_transport(const std::shared_ptr<adios2::Transport> &transport,
                                   haddr_t size)
{
    if (!transport)
    {
        return H5I_INVALID_HID;
    }

    const hid_t driverID = RegisterTransportVFD();
    if (driverID < 0)
    {
        return H5I_INVALID_HID;
    }

    const TransportFAPL properties{&transport, size};
    const hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    if (fapl < 0)
    {
        return H5I_INVALID_HID;
    }
    if (H5Pset_driver(fapl, driverID, &properties) < 0)
    {
        H5Pclose(fapl);
        return H5I_INVALID_HID;
    }
    return fapl;
}
