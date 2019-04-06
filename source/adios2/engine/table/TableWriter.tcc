/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.tcc implementation of template functions with known type
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */
#ifndef ADIOS2_ENGINE_TABLEWRITER_TCC_
#define ADIOS2_ENGINE_TABLEWRITER_TCC_

#include "TableWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void TableWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void TableWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    std::cout << " =========== table engine put" << std::endl;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
