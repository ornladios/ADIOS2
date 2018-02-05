/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.h
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SST_WRITER_TCC_
#define ADIOS2_ENGINE_SST_SST_WRITER_TCC_

#include "SstWriter.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

template <class T>
void SstWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);

    // This part will go away, this is just to monitor variables per rank

    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_FFSmarshal)
    {
        SstMarshal(m_Output, (void *)&variable, variable.m_Name.c_str(),
                   variable.m_Type.c_str(), variable.m_ElementSize,
                   variable.m_Shape.size(), variable.m_Shape.data(),
                   variable.m_Count.data(), variable.m_Start.data(), values);
    }
    else
    {
        // Do BP marshaling
    }
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_H_ */
