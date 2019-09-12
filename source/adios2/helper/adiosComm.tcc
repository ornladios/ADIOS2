/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.tcc : specialization of template functions defined in
 * adiosComm.h
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_TCC_
#define ADIOS2_HELPER_ADIOSCOMM_TCC_

#include "adiosComm.h"

#include "adios2/common/ADIOSMPI.h"

namespace adios2
{
namespace helper
{

// BroadcastValue full specializations forward-declared in 'adiosComm.inl'.
template <>
size_t Comm::BroadcastValue(const size_t &input, const int rankSource) const
{
    size_t output = 0;
    if (rankSource == this->Rank())
    {
        output = input;
    }

    this->Bcast(&output, 1, rankSource);

    return output;
}

template <>
std::string Comm::BroadcastValue(const std::string &input,
                                 const int rankSource) const
{
    const size_t inputSize = input.size();
    const size_t length = this->BroadcastValue(inputSize, rankSource);
    std::string output;

    if (rankSource == this->Rank())
    {
        output = input;
    }
    else
    {
        output.resize(length);
    }

    this->Bcast(const_cast<char *>(output.data()), length, rankSource);

    return output;
}

// BroadcastVector full specializations forward-declared in 'adiosComm.inl'.
template <>
void Comm::BroadcastVector(std::vector<char> &vector,
                           const int rankSource) const
{
    if (this->Size() == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = this->BroadcastValue(vector.size(), rankSource);

    if (rankSource != this->Rank())
    {
        vector.resize(inputSize);
    }

    this->Bcast(vector.data(), inputSize, rankSource);
}

template <>
void Comm::BroadcastVector(std::vector<size_t> &vector,
                           const int rankSource) const
{
    if (this->Size() == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = this->BroadcastValue(vector.size(), rankSource);

    if (rankSource != this->Rank())
    {
        vector.resize(inputSize);
    }

    this->Bcast(vector.data(), inputSize, rankSource);
}

// Datatype full specializations forward-declared in 'adiosComm.inl'.
template <>
MPI_Datatype Comm::Datatype<signed char>()
{
    return MPI_SIGNED_CHAR;
}

template <>
MPI_Datatype Comm::Datatype<char>()
{
    return MPI_CHAR;
}

template <>
MPI_Datatype Comm::Datatype<short>()
{
    return MPI_SHORT;
}

template <>
MPI_Datatype Comm::Datatype<int>()
{
    return MPI_INT;
}

template <>
MPI_Datatype Comm::Datatype<long>()
{
    return MPI_LONG;
}

template <>
MPI_Datatype Comm::Datatype<unsigned char>()
{
    return MPI_UNSIGNED_CHAR;
}

template <>
MPI_Datatype Comm::Datatype<unsigned short>()
{
    return MPI_UNSIGNED_SHORT;
}

template <>
MPI_Datatype Comm::Datatype<unsigned int>()
{
    return MPI_UNSIGNED;
}

template <>
MPI_Datatype Comm::Datatype<unsigned long>()
{
    return MPI_UNSIGNED_LONG;
}

template <>
MPI_Datatype Comm::Datatype<unsigned long long>()
{
    return MPI_UNSIGNED_LONG_LONG;
}

template <>
MPI_Datatype Comm::Datatype<long long>()
{
    return MPI_LONG_LONG_INT;
}

template <>
MPI_Datatype Comm::Datatype<double>()
{
    return MPI_DOUBLE;
}

template <>
MPI_Datatype Comm::Datatype<long double>()
{
    return MPI_LONG_DOUBLE;
}

template <>
MPI_Datatype Comm::Datatype<std::pair<int, int>>()
{
    return MPI_2INT;
}

template <>
MPI_Datatype Comm::Datatype<std::pair<float, int>>()
{
    return MPI_FLOAT_INT;
}

template <>
MPI_Datatype Comm::Datatype<std::pair<double, int>>()
{
    return MPI_DOUBLE_INT;
}

template <>
MPI_Datatype Comm::Datatype<std::pair<long double, int>>()
{
    return MPI_LONG_DOUBLE_INT;
}

template <>
MPI_Datatype Comm::Datatype<std::pair<short, int>>()
{
    return MPI_SHORT_INT;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSCOMM_TCC_ */
