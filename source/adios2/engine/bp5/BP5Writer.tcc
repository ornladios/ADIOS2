/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.tcc implementation of template functions with known type
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */
#ifndef ADIOS2_ENGINE_BP5_BP5WRITER_TCC_
#define ADIOS2_ENGINE_BP5_BP5WRITER_TCC_

#include "BP5Writer.h"
#include "adios2/helper/adiosMath.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void BP5Writer::PutCommon(Variable<T> &variable, const T *values, bool sync)
{
    variable.SetData(values);

    size_t *Shape = NULL;
    size_t *Start = NULL;
    size_t *Count = NULL;
    size_t DimCount = 0;

    if (variable.m_ShapeID == ShapeID::GlobalArray)
    {
        DimCount = variable.m_Shape.size();
        Shape = variable.m_Shape.data();
        Start = variable.m_Start.data();
        Count = variable.m_Count.data();
    }
    else if (variable.m_ShapeID == ShapeID::LocalArray)
    {
        DimCount = variable.m_Count.size();
        Count = variable.m_Count.data();
    }

    if (!sync)
    {
        /* If arrays is small, force copying to internal buffer to aggregate
         * small writes */
        size_t n = helper::GetTotalSize(variable.m_Count) * sizeof(T);
        if (n < 4194304 /* 4MB */)
        {
            sync = true;
        }
    }
    m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                            variable.m_Type, variable.m_ElementSize, DimCount,
                            Shape, Count, Start, values, sync);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP5_BP5WRITER_TCC_ */
