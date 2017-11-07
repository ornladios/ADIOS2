/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.tcc
 *
 *  Created on: Sep 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_TCC_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1READER_TCC_

#include "ADIOS1Reader.h"

namespace adios2
{

/*
#define declare(T, L)                                                          \
    Variable<T> *ADIOS1Reader::DoInquireVariable##L(                           \
        const std::string &variableName)                                       \
    {                                                                          \
        return InquireVariableCommon<T>(variableName);                         \
    }

ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare
*/
} // end namespace adios2

#endif // ADIOS2_ENGINE_ADIOS1_ADIOS1READER_TCC_
