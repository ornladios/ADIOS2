/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.cpp
 */

#include "adiosComm.h"
#include "adiosComm.tcc"

#include <utility>

#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace helper
{

namespace
{
const size_t DatatypeToSize[] = {
    sizeof(signed char),
    sizeof(char),
    sizeof(short),
    sizeof(int),
    sizeof(long),
    sizeof(unsigned char),
    sizeof(unsigned short),
    sizeof(unsigned int),
    sizeof(unsigned long),
    sizeof(unsigned long long),
    sizeof(long long),
    sizeof(double),
    sizeof(long double),
    sizeof(std::pair<int, int>),
    sizeof(std::pair<float, int>),
    sizeof(std::pair<double, int>),
    sizeof(std::pair<long double, int>),
    sizeof(std::pair<short, int>),
};

size_t ToSize(CommImpl::Datatype dt) { return DatatypeToSize[int(dt)]; }
}

Comm::Comm() = default;

Comm::Comm(std::unique_ptr<CommImpl> impl) : m_Impl(std::move(impl)) {}

Comm::~Comm() = default;

Comm::Comm(Comm &&comm) = default;

Comm &Comm::operator=(Comm &&comm) = default;

void Comm::Free(const std::string &hint) { m_Impl->Free(hint); }

Comm Comm::Duplicate(const std::string &hint) const
{
    return Comm(m_Impl->Duplicate(hint));
}

Comm Comm::Split(int color, int key, const std::string &hint) const
{
    return Comm(m_Impl->Split(color, key, hint));
}

Comm Comm::World(const std::string &hint) const
{
    return Comm(m_Impl->World(hint));
}

int Comm::Rank() const { return m_Impl->Rank(); }

int Comm::Size() const { return m_Impl->Size(); }

void Comm::Barrier(const std::string &hint) const { m_Impl->Barrier(hint); }

std::string Comm::BroadcastFile(const std::string &fileName,
                                const std::string hint,
                                const int rankSource) const
{
    int rank = this->Rank();
    std::string fileContents;

    // Read the file on rank 0 and broadcast it to everybody else
    if (rank == rankSource)
    {
        // load file contents
        fileContents = FileToString(fileName, hint);
    }
    fileContents = this->BroadcastValue(fileContents, rankSource);

    return fileContents;
}

std::vector<size_t> Comm::GetGathervDisplacements(const size_t *counts,
                                                  const size_t countsSize)
{
    std::vector<size_t> displacements(countsSize);
    displacements[0] = 0;

    for (size_t i = 1; i < countsSize; ++i)
    {
        displacements[i] = displacements[i - 1] + counts[i - 1];
    }
    return displacements;
}

Comm::Req::Req() = default;

Comm::Req::Req(std::unique_ptr<CommReqImpl> impl) : m_Impl(std::move(impl)) {}

Comm::Req::~Req() = default;

Comm::Req::Req(Req &&req) = default;

Comm::Req &Comm::Req::operator=(Req &&req) = default;

Comm::Status Comm::Req::Wait(const std::string &hint)
{
    Comm::Status status;
    if (m_Impl)
    {
        status = m_Impl->Wait(hint);
        m_Impl.reset();
    }
    return status;
}

CommImpl::~CommImpl() = default;

size_t CommImpl::SizeOf(Datatype datatype) { return ToSize(datatype); }

Comm CommImpl::MakeComm(std::unique_ptr<CommImpl> impl)
{
    return Comm(std::move(impl));
}

Comm::Req CommImpl::MakeReq(std::unique_ptr<CommReqImpl> impl)
{
    return Comm::Req(std::move(impl));
}

CommImpl *CommImpl::Get(Comm const &comm) { return comm.m_Impl.get(); }

CommReqImpl::~CommReqImpl() = default;

} // end namespace helper
} // end namespace adios2
