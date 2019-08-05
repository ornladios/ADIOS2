/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.h : Communicate in a multi-process environment.
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_H_
#define ADIOS2_HELPER_ADIOSCOMM_H_

#include "adios2/common/ADIOSMPI.h"

#include <string>
#include <vector>

namespace adios2
{
namespace helper
{

/** @brief Encapsulation for communication in a multi-process environment.  */
class Comm
{
public:
    class Req;
    class Status;

    /**
     * @brief Default constructor.  Produces an empty communicator.
     *
     * An empty communicator may not be used for communcation.
     */
    Comm();

    /**
     * @brief Move constructor.  Moves communicator state from that given.
     *
     * The moved-from communicator is left empty and may not be used for
     * communication.
     */
    Comm(Comm &&);

    /**
     * @brief Deleted copy constructor.  A communicator may not be copied.
     */
    Comm(Comm const &) = delete;

    ~Comm();

    /**
     * @brief Move assignment.  Moves communicator state from that given.
     *
     * The moved-from communicator is left empty and may not be used for
     * communication.
     */
    Comm &operator=(Comm &&);

    /**
     * @brief Deleted copy assignment.  A communicator may not be copied.
     */
    Comm &operator=(Comm const &) = delete;

    /**
     * @brief Swap communicator state with another.
     */
    void swap(Comm &comm);

    // FIXME: Remove conversion after clients transition to encapsulation.
    /** Convert to a concrete MPI communicator.  */
    operator MPI_Comm() const { return m_MPIComm; }

    /**
     * @brief Create a communicator by duplicating a MPI communicator.
     */
    static Comm Duplicate(MPI_Comm mpiComm);

    /**
     * @brief Free the communicator.
     * @param hint Description of std::runtime_error exception on error.
     *
     * The communicator is left empty and may not be used for communication.
     */
    void Free(const std::string &hint = std::string());

    /**
     * @brief Split the communicator.
     * @param color Control of subset assignment (nonnegative integer).
     * @param key Control of rank assignment (integer).
     * @param hint Description of std::runtime_error exception on error.
     *
     * Creates a new communicator covering the subset of the original
     * processes that pass the same 'color'.  Ranks assigned by 'key' order.
     */
    Comm Split(int color, int key,
               const std::string &hint = std::string()) const;

    int Rank() const;
    int Size() const;

    void Barrier(const std::string &hint = std::string()) const;

    /**
     * Gather a single source value from each ranks and forms a vector in
     * rankDestination.
     * @param rankDestination root, where all sizes are gathered in the returned
     * vector
     * @return in rankDestination: aggregated vector<T>, others: empty
     */
    template <class T>
    std::vector<T> GatherValues(T source, int rankDestination = 0) const;

    /**
     * Gather equal size arrays
     * @param source
     * @param sourceCount
     * @param destination
     * @param rankDestination
     */
    template <class T>
    void GatherArrays(const T *source, size_t sourceCount, T *destination,
                      int rankDestination = 0) const;

    /**
     * Gather arrays of the same type into a destination (must be pre-allocated)
     * if countsSize == 1, calls MPI_Gather, otherwise calls MPI_Gatherv.
     * This function must be specialized for each MPI_Type.
     * @param source  input from each rank
     * @param counts  counts for each source
     * @param countsSize number of counts
     * @param destination resulting gathered buffer in rankDestination, unchaged
     * in others
     * @param rankDestination rank in which arrays are gathered (root)
     */
    template <class T>
    void GathervArrays(const T *source, size_t sourceCount,
                       const size_t *counts, size_t countsSize, T *destination,
                       int rankDestination = 0) const;

    template <class T>
    void GathervVectors(const std::vector<T> &in, std::vector<T> &out,
                        size_t &position, int rankDestination = 0,
                        size_t extraSize = 0) const;
    /**
     * Perform AllGather for source value
     * @param source input
     * @return in all ranks: a vector with gathered source values ordered per
     * rank
     */
    template <class T>
    std::vector<T> AllGatherValues(const T source) const;

    /**
     * Perform AllGather for equal size arrays
     * @param source
     * @param sourceCount
     * @param destination
     */
    template <class T>
    void AllGatherArrays(const T *source, const size_t sourceCount,
                         T *destination) const;

    template <class T>
    T ReduceValues(const T source, MPI_Op operation = MPI_SUM,
                   const int rankDestination = 0) const;

    template <class T>
    T BroadcastValue(const T &input, const int rankSource = 0) const;

    template <class T>
    void BroadcastVector(std::vector<T> &vector,
                         const int rankSource = 0) const;

    std::string BroadcastFile(const std::string &fileName,
                              const std::string hint = "",
                              const int rankSource = 0) const;

    template <typename TSend, typename TRecv>
    void Allgather(const TSend *sendbuf, size_t sendcount, TRecv *recvbuf,
                   size_t recvcount,
                   const std::string &hint = std::string()) const;

    template <typename T>
    void Allreduce(const T *sendbuf, T *recvbuf, size_t count, MPI_Op op,
                   const std::string &hint = std::string()) const;

    template <typename T>
    void Bcast(T *buffer, size_t count, int root,
               const std::string &hint = std::string()) const;

    template <typename TSend, typename TRecv>
    void Gather(const TSend *sendbuf, size_t sendcount, TRecv *recvbuf,
                size_t recvcount, int root,
                const std::string &hint = std::string()) const;

    template <typename T>
    void Reduce(const T *sendbuf, T *recvbuf, size_t count, MPI_Op op, int root,
                const std::string &hint = std::string()) const;

    template <typename T>
    void ReduceInPlace(T *buf, size_t count, MPI_Op op, int root,
                       const std::string &hint = std::string()) const;

    template <typename T>
    void Send(const T *buf, size_t count, int dest, int tag,
              const std::string &hint = std::string()) const;

    template <typename T>
    Status Recv(T *buf, size_t count, int source, int tag,
                const std::string &hint = std::string()) const;

    template <typename TSend, typename TRecv>
    void Scatter(const TSend *sendbuf, size_t sendcount, TRecv *recvbuf,
                 size_t recvcount, int root,
                 const std::string &hint = std::string()) const;

    template <typename T>
    Req Isend(const T *buffer, const size_t count, int destination, int tag,
              const std::string &hint = std::string()) const;

    template <typename T>
    Req Irecv(T *buffer, const size_t count, int source, int tag,
              const std::string &hint = std::string()) const;

private:
    /**
     * @brief Construct by taking ownership of a MPI communicator.
     *
     * This is a private implementation detail used by static
     * methods like Duplicate.
     */
    explicit Comm(MPI_Comm mpiComm);

    /** Encapsulated MPI communicator instance.  */
    MPI_Comm m_MPIComm = MPI_COMM_NULL;

    static void CheckMPIReturn(const int value, const std::string &hint);

    void AllgatherImpl(const void *sendbuf, size_t sendcount,
                       MPI_Datatype sendtype, void *recvbuf, size_t recvcount,
                       MPI_Datatype recvtype, const std::string &hint) const;

    void AllreduceImpl(const void *sendbuf, void *recvbuf, size_t count,
                       MPI_Datatype datatype, MPI_Op op,
                       const std::string &hint) const;

    void BcastImpl(void *buffer, size_t count, MPI_Datatype datatype, int root,
                   const std::string &hint) const;

    void GatherImpl(const void *sendbuf, size_t sendcount,
                    MPI_Datatype sendtype, void *recvbuf, size_t recvcount,
                    MPI_Datatype recvtype, int root,
                    const std::string &hint) const;

    void ReduceImpl(const void *sendbuf, void *recvbuf, size_t count,
                    MPI_Datatype datatype, MPI_Op op, int root,
                    const std::string &hint) const;

    void ReduceInPlaceImpl(void *buf, size_t count, MPI_Datatype datatype,
                           MPI_Op op, int root, const std::string &hint) const;

    void SendImpl(const void *buf, size_t count, MPI_Datatype datatype,
                  int dest, int tag, const std::string &hint) const;

    Status RecvImpl(void *buf, size_t count, MPI_Datatype datatype, int source,
                    int tag, const std::string &hint) const;

    void ScatterImpl(const void *sendbuf, size_t sendcount,
                     MPI_Datatype sendtype, void *recvbuf, size_t recvcount,
                     MPI_Datatype recvtype, int root,
                     const std::string &hint) const;

    Req IsendImpl(const void *buffer, size_t count, MPI_Datatype datatype,
                  int dest, int tag, const std::string &hint) const;

    Req IrecvImpl(void *buffer, size_t count, MPI_Datatype datatype, int source,
                  int tag, const std::string &hint) const;

    /** Return MPI datatype id for type T.  */
    template <typename T>
    static MPI_Datatype Datatype();
};

class Comm::Req
{
public:
    /**
     * @brief Default constructor.  Produces an empty request.
     *
     * An empty request may not be used.
     */
    Req();

    /**
     * @brief Move constructor.  Moves request state from that given.
     *
     * The moved-from request is left empty and may not be used.
     */
    Req(Req &&);

    /**
     * @brief Deleted copy constructor.  A request may not be copied.
     */
    Req(Req const &) = delete;

    ~Req();

    /**
     * @brief Move assignment.  Moves request state from that given.
     *
     * The moved-from request is left empty and may not be used.
     */
    Req &operator=(Req &&);

    /**
     * @brief Deleted copy assignment.  A request may not be copied.
     */
    Req &operator=(Req const &) = delete;

    /**
     * @brief Swap request state with another.
     */
    void swap(Req &req);

    /**
     * @brief Wait for the request to finish.
     *
     * On return, the request is empty.
     */
    Comm::Status Wait(const std::string &hint = std::string());

private:
    friend class Comm;

    Req(MPI_Datatype datatype);

    /** Encapsulated MPI datatype of the requested operation.  */
    MPI_Datatype m_MPIDatatype = MPI_DATATYPE_NULL;

    /** Encapsulated MPI request instances.  There may be more than
     *  one when we batch requests too large for MPI interfaces.  */
    std::vector<MPI_Request> m_MPIReqs;
};

class Comm::Status
{
public:
    /**
     * @brief The index of the process from which a message was received.
     */
    int Source = -1;

    /**
     * @brief The tag distinguishing the message for the application.
     */
    int Tag = -1;

    /**
     * @brief The number of elements received in a message.
     */
    size_t Count = 0;

    /**
     * @brief True if this is the status of a cancelled operation.
     */
    bool Cancelled = false;
};

} // end namespace helper
} // end namespace adios2

#include "adiosComm.inl"

#endif // ADIOS2_HELPER_ADIOSCOMM_H_
