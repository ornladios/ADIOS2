/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.tcc implementation of template functions with known type
 *
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
    if (!m_BetweenStepPairs)
    {
        BeginStep(StepMode::Update);
    }
    variable.SetData(values);

    size_t *Shape = NULL;
    size_t *Start = NULL;
    size_t *Count = NULL;
    size_t DimCount = variable.m_Count.size();

    if (variable.m_ShapeID == ShapeID::GlobalArray)
    {
        Shape = variable.m_Shape.data();
        Count = variable.m_Count.data();
        Start = variable.m_Start.data();
    }
    else if (variable.m_ShapeID == ShapeID::LocalArray)
    {
        Count = variable.m_Count.data();
    }

    if (!sync)
    {
        /* If arrays is small, force copying to internal buffer to aggregate
         * small writes */
        size_t n = helper::GetTotalSize(variable.m_Count) * sizeof(T);
        if (n < m_Parameters.MinDeferredSize)
        {
            sync = true;
        }
    }

    if (!variable.m_MemoryCount.empty())
    {
        // get a temporary span then fill with memselection now
        format::BufferV::BufferPos bp5span(0, 0, 0);
        m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                                variable.m_Type, variable.m_ElementSize,
                                DimCount, Shape, Count, Start, nullptr, false,
                                &bp5span);
        T *ptr = reinterpret_cast<T *>(
            m_BP5Serializer.GetPtr(bp5span.bufferIdx, bp5span.posInBuffer));

        const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);

        helper::CopyMemoryBlock(
            ptr, variable.m_Start, variable.m_Count, sourceRowMajor, values,
            variable.m_Start, variable.m_Count, sourceRowMajor, false, Dims(),
            Dims(), variable.m_MemoryStart, variable.m_MemoryCount);
    }
    else
    {
        if (std::is_same<T, std::string>::value)
        {
            std::string &source = *(std::string *)values;
            void *p = &(source[0]);
            m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                                    variable.m_Type, variable.m_ElementSize,
                                    DimCount, Shape, Count, Start, &p, sync,
                                    nullptr);
        }
        else
            m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                                    variable.m_Type, variable.m_ElementSize,
                                    DimCount, Shape, Count, Start, values, sync,
                                    nullptr);
    }
}

template <class T>
void BP5Writer::PutCommonSpan(Variable<T> &variable,
                              typename Variable<T>::Span &span,
                              const bool initialize, const T &value)
{
    format::BufferV::BufferPos bp5span(0, 0, 0);

    size_t *Shape = NULL;
    size_t *Start = NULL;
    size_t *Count = NULL;
    size_t DimCount = 0;

    if (!m_BetweenStepPairs)
    {
        BeginStep(StepMode::Update);
    }
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

    if (std::is_same<T, std::string>::value)
    {
        m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                                variable.m_Type, variable.m_ElementSize,
                                DimCount, Shape, Count, Start, nullptr, false,
                                &bp5span);
    }
    else
        m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(),
                                variable.m_Type, variable.m_ElementSize,
                                DimCount, Shape, Count, Start, nullptr, false,
                                &bp5span);

    span.m_PayloadPosition = bp5span.posInBuffer;
    span.m_BufferIdx = bp5span.bufferIdx;
    span.m_Value = value;

    /* initialize buffer if needed */
    if (initialize)
    {
        const size_t ElemCount = m_BP5Serializer.CalcSize(DimCount, Count);
        T *itBegin = reinterpret_cast<T *>(
            m_BP5Serializer.GetPtr(span.m_BufferIdx, span.m_PayloadPosition));

        // TODO from BP4: does std::fill_n have a bug in gcc or due to
        // optimizations this is impossible due to memory alignment? This seg
        // faults in Release mode only . Even RelWithDebInfo works, replacing
        // with explicit loop below using access operator []
        // std::fill_n(itBegin, blockSize, span->m_Value);

        for (size_t i = 0; i < ElemCount; ++i)
        {
            itBegin[i] = value;
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP5_BP5WRITER_TCC_ */
