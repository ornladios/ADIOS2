/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosCommDummy.cpp
 */

#include "adiosCommDummy.h"

#include <cstring>
#include <iostream>

#include "adiosComm.h"

namespace adios2
{
namespace helper
{

namespace
{
void CommDummyError(const std::string &msg)
{
    std::cerr << "CommDummy: a function returned error code '" << msg
              << "'. Aborting!" << std::endl;
    std::abort();
}
}

class CommReqImplDummy : public CommReqImpl
{
public:
    CommReqImplDummy() {}
    ~CommReqImplDummy() override;

    Comm::Status Wait(const std::string &hint) override;
};

CommReqImplDummy::~CommReqImplDummy() = default;

class CommImplDummy : public CommImpl
{
public:
    CommImplDummy() = default;
    ~CommImplDummy() override;

    void Free(const std::string &hint) override;
    std::unique_ptr<CommImpl> Duplicate(const std::string &hint) const override;
    std::unique_ptr<CommImpl> Split(int color, int key,
                                    const std::string &hint) const override;
    std::unique_ptr<CommImpl> World(const std::string &hint) const override;

    int Rank() const override;
    int Size() const override;
    bool IsMPI() const override;
    void Barrier(const std::string &hint) const override;

    void Allgather(const void *sendbuf, size_t sendcount, Datatype sendtype,
                   void *recvbuf, size_t recvcount, Datatype recvtype,
                   const std::string &hint) const override;

    void Allgatherv(const void *sendbuf, size_t sendcount, Datatype sendtype,
                    void *recvbuf, const size_t *recvcounts,
                    const size_t *displs, Datatype recvtype,
                    const std::string &hint) const override;

    void Allreduce(const void *sendbuf, void *recvbuf, size_t count,
                   Datatype datatype, Comm::Op op,
                   const std::string &hint) const override;

    void Bcast(void *buffer, size_t count, Datatype datatype, int root,
               const std::string &hint) const override;

    void Gather(const void *sendbuf, size_t sendcount, Datatype sendtype,
                void *recvbuf, size_t recvcount, Datatype recvtype, int root,
                const std::string &hint) const override;

    void Gatherv(const void *sendbuf, size_t sendcount, Datatype sendtype,
                 void *recvbuf, const size_t *recvcounts, const size_t *displs,
                 Datatype recvtype, int root,
                 const std::string &hint) const override;

    void Gatherv64(const void *sendbuf, size_t sendcount, Datatype sendtype,
                   void *recvbuf, const size_t *recvcounts,
                   const size_t *displs, Datatype recvtype, int root,
                   const std::string &hint) const override;

    void Gatherv64OneSidedPush(const void *sendbuf, size_t sendcount,
                               Datatype sendtype, void *recvbuf,
                               const size_t *recvcounts, const size_t *displs,
                               Datatype recvtype, int root,
                               const std::string &hint) const override;

    void Gatherv64OneSidedPull(const void *sendbuf, size_t sendcount,
                               Datatype sendtype, void *recvbuf,
                               const size_t *recvcounts, const size_t *displs,
                               Datatype recvtype, int root,
                               const std::string &hint) const override;

    void Reduce(const void *sendbuf, void *recvbuf, size_t count,
                Datatype datatype, Comm::Op op, int root,
                const std::string &hint) const override;

    void ReduceInPlace(void *buf, size_t count, Datatype datatype, Comm::Op op,
                       int root, const std::string &hint) const override;

    void Send(const void *buf, size_t count, Datatype datatype, int dest,
              int tag, const std::string &hint) const override;

    Comm::Status Recv(void *buf, size_t count, Datatype datatype, int source,
                      int tag, const std::string &hint) const override;

    void Scatter(const void *sendbuf, size_t sendcount, Datatype sendtype,
                 void *recvbuf, size_t recvcount, Datatype recvtype, int root,
                 const std::string &hint) const override;

    Comm::Req Isend(const void *buffer, size_t count, Datatype datatype,
                    int dest, int tag, const std::string &hint) const override;

    Comm::Req Irecv(void *buffer, size_t count, Datatype datatype, int source,
                    int tag, const std::string &hint) const override;
};

CommImplDummy::~CommImplDummy() = default;

void CommImplDummy::Free(const std::string &) {}

std::unique_ptr<CommImpl> CommImplDummy::Duplicate(const std::string &) const
{
    return std::unique_ptr<CommImpl>(new CommImplDummy());
}

std::unique_ptr<CommImpl> CommImplDummy::Split(int, int,
                                               const std::string &) const
{
    return std::unique_ptr<CommImpl>(new CommImplDummy());
}

std::unique_ptr<CommImpl> CommImplDummy::World(const std::string &) const
{
    return std::unique_ptr<CommImpl>(new CommImplDummy());
}

int CommImplDummy::Rank() const { return 0; }

int CommImplDummy::Size() const { return 1; }

bool CommImplDummy::IsMPI() const { return false; }

void CommImplDummy::Barrier(const std::string &) const {}

void CommImplDummy::Allgather(const void *sendbuf, size_t sendcount,
                              Datatype sendtype, void *recvbuf,
                              size_t recvcount, Datatype recvtype,
                              const std::string &hint) const
{
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, 0, hint);
}

void CommImplDummy::Allgatherv(const void *sendbuf, size_t sendcount,
                               Datatype sendtype, void *recvbuf,
                               const size_t *recvcounts, const size_t *displs,
                               Datatype recvtype, const std::string &hint) const
{
    const size_t recvcount = recvcounts[0];
    if (recvcount != sendcount)
    {
        return CommDummyError("send and recv counts differ");
    }
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, 0, hint);
}

void CommImplDummy::Allreduce(const void *sendbuf, void *recvbuf, size_t count,
                              Datatype datatype, Comm::Op op,
                              const std::string &hint) const
{
    CommImplDummy::Reduce(sendbuf, recvbuf, count, datatype, op, 0, hint);
}

void CommImplDummy::Bcast(void *, size_t, Datatype, int,
                          const std::string &) const
{
}

void CommImplDummy::Gather(const void *sendbuf, size_t sendcount,
                           Datatype sendtype, void *recvbuf, size_t recvcount,
                           Datatype recvtype, int root,
                           const std::string &) const
{
    if (sendcount > 0 && !sendbuf)
    {
        return CommDummyError("sendbuf is null");
    }
    if (recvcount > 0 && !recvbuf)
    {
        return CommDummyError("recvbuf is null");
    }
    if (root != 0)
    {
        return CommDummyError("root is not 0");
    }

    const size_t nsent = sendcount * CommImpl::SizeOf(sendtype);
    const size_t nrecv = recvcount * CommImpl::SizeOf(recvtype);

    if (nrecv != nsent)
    {
        return CommDummyError("send and recv sizes differ");
    }

    std::memcpy(recvbuf, sendbuf, nsent);
}

void CommImplDummy::Gatherv(const void *sendbuf, size_t sendcount,
                            Datatype sendtype, void *recvbuf,
                            const size_t *recvcounts, const size_t *displs,
                            Datatype recvtype, int root,
                            const std::string &hint) const
{
    const size_t recvcount = recvcounts[0];
    if (recvcount != sendcount)
    {
        return CommDummyError("send and recv counts differ");
    }
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, hint);
}

void CommImplDummy::Gatherv64(const void *sendbuf, size_t sendcount,
                              Datatype sendtype, void *recvbuf,
                              const size_t *recvcounts, const size_t *displs,
                              Datatype recvtype, int root,
                              const std::string &hint) const
{
    const size_t recvcount = recvcounts[0];
    if (recvcount != sendcount)
    {
        return CommDummyError("send and recv counts differ");
    }
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, hint);
}

void CommImplDummy::Gatherv64OneSidedPush(const void *sendbuf, size_t sendcount,
                                          Datatype sendtype, void *recvbuf,
                                          const size_t *recvcounts,
                                          const size_t *displs,
                                          Datatype recvtype, int root,
                                          const std::string &hint) const
{
    const size_t recvcount = recvcounts[0];
    if (recvcount != sendcount)
    {
        return CommDummyError("send and recv counts differ");
    }
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, hint);
}

void CommImplDummy::Gatherv64OneSidedPull(const void *sendbuf, size_t sendcount,
                                          Datatype sendtype, void *recvbuf,
                                          const size_t *recvcounts,
                                          const size_t *displs,
                                          Datatype recvtype, int root,
                                          const std::string &hint) const
{
    const size_t recvcount = recvcounts[0];
    if (recvcount != sendcount)
    {
        return CommDummyError("send and recv counts differ");
    }
    CommImplDummy::Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, hint);
}

void CommImplDummy::Reduce(const void *sendbuf, void *recvbuf, size_t count,
                           Datatype datatype, Comm::Op, int,
                           const std::string &) const
{
    std::memcpy(recvbuf, sendbuf, count * CommImpl::SizeOf(datatype));
}

void CommImplDummy::ReduceInPlace(void *, size_t, Datatype, Comm::Op, int,
                                  const std::string &) const
{
}

void CommImplDummy::Send(const void *, size_t, Datatype, int, int,
                         const std::string &) const
{
}

Comm::Status CommImplDummy::Recv(void *, size_t, Datatype, int, int,
                                 const std::string &) const
{
    Comm::Status status;
    return status;
}

void CommImplDummy::Scatter(const void *sendbuf, size_t sendcount,
                            Datatype sendtype, void *recvbuf, size_t recvcount,
                            Datatype recvtype, int root,
                            const std::string &) const
{
    if (sendcount > 0 && !sendbuf)
    {
        return CommDummyError("sendbuf is null");
    }
    if (recvcount > 0 && !recvbuf)
    {
        return CommDummyError("recvbuf is null");
    }
    if (root != 0)
    {
        return CommDummyError("root is not 0");
    }

    const size_t nsent = sendcount * CommImpl::SizeOf(sendtype);
    const size_t nrecv = recvcount * CommImpl::SizeOf(recvtype);

    if (nrecv != nsent)
    {
        return CommDummyError("send and recv sizes differ");
    }

    std::memcpy(recvbuf, sendbuf, nsent);
}

Comm::Req CommImplDummy::Isend(const void *, size_t, Datatype, int, int,
                               const std::string &) const
{
    auto req = std::unique_ptr<CommReqImplDummy>(new CommReqImplDummy());
    return MakeReq(std::move(req));
}

Comm::Req CommImplDummy::Irecv(void *, size_t, Datatype, int, int,
                               const std::string &) const
{
    auto req = std::unique_ptr<CommReqImplDummy>(new CommReqImplDummy());
    return MakeReq(std::move(req));
}

Comm::Status CommReqImplDummy::Wait(const std::string &hint)
{
    Comm::Status status;
    return status;
}

Comm CommDummy()
{
    auto comm = std::unique_ptr<CommImpl>(new CommImplDummy());
    return CommImpl::MakeComm(std::move(comm));
}

} // end namespace helper
} // end namespace adios2
