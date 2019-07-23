/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  NullTransport.cpp
 *
 *  Created on: Apr 17 2019
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#include "NullTransport.h"

#include <cstring> // std::memset

namespace adios2
{
namespace transport
{

struct NullTransport::NullTransportImpl
{
    bool IsOpen = false;
    size_t CurPos = 0;
    size_t Capacity = 0;
};

NullTransport::NullTransport(helper::Comm const &comm, const bool debugMode)
: Transport("NULL", "NULL", comm, debugMode), Impl(new NullTransportImpl)
{
}

NullTransport::~NullTransport() = default;

void NullTransport::Open(const std::string &name, const Mode openMode)
{
    if (Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::Open: The transport is already open.");
    }

    ProfilerStart("open");
    Impl->IsOpen = true;
    Impl->CurPos = 0;
    Impl->Capacity = 0;
    ProfilerStop("open");
}

void NullTransport::SetBuffer(char *buffer, size_t size) { return; }

void NullTransport::Write(const char *buffer, size_t size, size_t start)
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::Write: The transport is not open.");
    }

    ProfilerStart("write");
    Impl->CurPos = start + size;
    if (Impl->CurPos > Impl->Capacity)
    {
        Impl->Capacity = Impl->CurPos;
    }
    ProfilerStop("write");
}

void NullTransport::Read(char *buffer, size_t size, size_t start)
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::Read: The transport is not open.");
    }

    ProfilerStart("read");
    if (start + size > Impl->Capacity)
    {
        throw std::out_of_range(
            "ERROR: NullTransport::Read: size+start exceeds capacity");
    }
    std::memset(buffer, 0, size);
    Impl->CurPos = start + size;
    ProfilerStop("read");
}

size_t NullTransport::GetSize() { return Impl->Capacity; }

void NullTransport::Flush()
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::Flush: The transport is not open.");
    }
}

void NullTransport::Close()
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::Close: The transport is not open.");
    }

    Impl->CurPos = 0;
    Impl->Capacity = 0;
    Impl->IsOpen = false;
}

void NullTransport::SeekToEnd()
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::SeekToEnd: The transport is not open.");
    }
    Impl->CurPos = Impl->Capacity - 1;
}

void NullTransport::SeekToBegin()
{
    if (!Impl->IsOpen)
    {
        throw std::runtime_error(
            "ERROR: NullTransport::SeekToEnd: The transport is not open.");
    }
    Impl->CurPos = 0;
}

void NullTransport::MkDir(const std::string &fileName) { return; }

void NullTransport::CheckName() const { return; }

} // end namespace transport
} // end namespace adios2
