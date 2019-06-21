/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.h : Communicate in a multi-process environment.
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_H_
#define ADIOS2_HELPER_ADIOSCOMM_H_

#include "adios2/common/ADIOSMPI.h"

namespace adios2
{
namespace helper
{

/** @brief Encapsulation for communication in a multi-process environment.  */
class Comm
{
public:
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
};

} // end namespace helper
} // end namespace adios2

#include "adiosComm.inl"

#endif // ADIOS2_HELPER_ADIOSCOMM_H_
