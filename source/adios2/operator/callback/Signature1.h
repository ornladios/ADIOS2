/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Signature1.h
 *
 *  Created on: Oct 19, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_CALLBACK_SIGNATURE1_H_
#define ADIOS2_OPERATOR_CALLBACK_SIGNATURE1_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace callback
{

template <class T>
class Signature1 : public Operator
{

public:
    Signature1<T>(const std::function<
                      void(const T *, const std::string &, const std::string &,
                           const std::string &, const Dims &)> &function,
                  const Params &parameters, const bool debugMode);

    ~Signature1<T>() = default;

    void RunCallback1(const T *, const std::string &, const std::string &,
                      const std::string &, const Dims &) final;

private:
    std::function<void(const T *, const std::string &, const std::string &,
                       const std::string &, const Dims &)>
        m_Function;
};

} // end namespace callback
} // end namespace adios2

#include <adios2/operator/callback/Signature1.inl>

#endif /* ADIOS2_OPERATOR_CALLBACK_CALLBACK1_H_ */
