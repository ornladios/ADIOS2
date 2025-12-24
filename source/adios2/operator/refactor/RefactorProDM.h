/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * RefactorProDM.h :
 *
 *  Created on: Oct 22, 2025
 *      Author: Qirui Tian <qt2@njit.edu>
 */

#ifndef ADIOS2_OPERATOR_REFACTOR_REFACTORPRODM_H_
#define ADIOS2_OPERATOR_REFACTOR_REFACTORPRODM_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace refactor
{

class RefactorProDM : public Operator
{
public:
    explicit RefactorProDM(const Params &parameters);
    ~RefactorProDM() = default;

    size_t GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                            const size_t ndims,
                            const size_t *dims) const override;

    size_t Operate(const char *dataIn, const Dims &blockStart,
                   const Dims &blockCount, const DataType type,
                   char *bufferOut) override;

    size_t InverseOperate(const char *bufferIn, const size_t sizeIn,
                          char *dataOut) override;

    size_t GetHeaderSize() const override;

    bool IsDataTypeValid(const DataType type) const override;

private:
    size_t Reconstruct(const char *bufferIn, const size_t sizeIn,
                         char *dataOut);

    size_t headerSize = 0;
};

} // end namespace refactor
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_REFACTOR_REFACTORPRODM_H_ */

