/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/Operator.h"
#include "adios2/core/VariableBase.h"
#include <memory>

namespace adios2
{
namespace core
{

std::string OperatorTypeToString(const Operator::OperatorType type);

std::shared_ptr<Operator> MakeOperator(const std::string &type, const Params &parameters);

size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut, MemorySpace memSpace,
                  std::shared_ptr<Operator> op = nullptr, Engine *engine = nullptr,
                  VariableBase *var = nullptr);

Params CreateOperatorParams(const Engine *engine, const VariableBase *variable);

} // end namespace core
} // end namespace adios2
