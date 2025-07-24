/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TimeSeriesReader.tcc
 *
 *  Created on: Apr 30, 2025
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_TIMESERIESREADER_TCC_
#define ADIOS2_ENGINE_TIMESERIESREADER_TCC_

#include "TimeSeriesReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
inline Variable<T> TimeSeriesReader::DuplicateVariable(Variable<T> *variable, IO &io, size_t ioIdx,
                                                       size_t engineIdx, T min, T max)
{
    auto it = m_VarInternalInfo.find(variable->m_Name);
    if (it != m_VarInternalInfo.end())
    {
        auto &vii = it->second;
        adios2::core::Variable<T> *v = io.InquireVariable<T>(variable->m_Name);

        if (v->m_ShapeID != variable->m_ShapeID)
        {
            helper::Throw<std::invalid_argument>(
                "Engine", "TimeSeriesReader", "DuplicateVariable",
                "Variable shape mismatch in separate files, existing variable shape ID is " +
                    ToString(v->m_ShapeID) + ", new shape ID is " + ToString(variable->m_ShapeID));
        }

        if (v->m_Shape.size() != variable->Shape().size())
        {
            helper::Throw<std::invalid_argument>(
                "Engine", "TimeSeriesReader", "DuplicateVariable",
                "Variable shape mismatch in separate files, existing variable number of "
                "dimensions is " +
                    std::to_string(v->m_Shape.size()) +
                    ", new variable's number of dimensions is " +
                    std::to_string(variable->Shape().size()));
        }

        size_t lastStep = vii.info.back().endStep;
        VarInternalInfoSingle viis(static_cast<void *>(variable), ioIdx, engineIdx);
        viis.stepCount = variable->GetAvailableStepsCount();
        viis.startStep = lastStep + 1;
        viis.endStep = viis.stepCount + viis.startStep - 1;

        if (engineIdx == vii.nEngines - 1)
        {
            vii.info[engineIdx] = viis;
        }
        else
        {
            vii.nEngines++;
            vii.info.push_back(viis);
        }

        for (size_t i = 0; i < viis.stepCount; ++i)
        {
            auto it = variable->m_AvailableShapes.find(i + 1);
            if (it != variable->m_AvailableShapes.end())
            {
                v->m_AvailableShapes[viis.startStep + i + 1] = variable->m_AvailableShapes[i + 1];
            }
        }
        v->m_AvailableStepsCount += viis.stepCount;

        if (TypeHasMinMax(variable->m_Type))
        {
            if (helper::LessThan(min, v->m_Min))
            {
                v->m_Min = min;
            }
            if (helper::GreaterThan(max, v->m_Max))
            {
                v->m_Max = max;
            }
        }

        // std::cout << "Updated variable " << v->m_Name << " from engine " << engineIdx
        //           << " with shape " << helper::DimsToString(v->m_Shape) << "\n";

        return *v;
    }
    else
    {
        adios2::core::Variable<T> &v = io.DefineVariable<T>(variable->m_Name, variable->Shape());
        v.m_AvailableStepsCount = variable->GetAvailableStepsCount();
        v.m_AvailableStepsStart = variable->GetAvailableStepsStart();
        v.m_ShapeID = variable->m_ShapeID;
        // v.m_Shape = variable->m_Shape;
        v.m_SingleValue = variable->m_SingleValue;
        v.m_ReadAsJoined = variable->m_ReadAsJoined;
        v.m_ReadAsLocalValue = variable->m_ReadAsLocalValue;
        v.m_RandomAccess = variable->m_RandomAccess;
        v.m_MemSpace = variable->m_MemSpace;
        v.m_JoinedDimPos = variable->m_JoinedDimPos;
        v.m_AvailableStepBlockIndexOffsets = variable->m_AvailableStepBlockIndexOffsets;
        v.m_AvailableShapes = variable->m_AvailableShapes;
        if (TypeHasMinMax(variable->m_Type))
        {
            v.m_Min = min;
            v.m_Max = max;
        }
        v.m_Value = variable->m_Value;
        v.m_StepsStart = variable->m_StepsStart;
        v.m_StepsCount = variable->m_StepsCount;
        v.m_Start = variable->m_Start;
        v.m_Count = variable->m_Count;
        v.m_AccuracyRequested = variable->m_AccuracyRequested;
        v.m_AccuracyProvided = variable->m_AccuracyProvided;
        v.m_FirstStreamingStep = variable->m_FirstStreamingStep;

        v.m_Engine = this; // Variable::Shape() uses this member to call engine
        VarInternalInfoSingle viis(static_cast<void *>(variable), ioIdx, engineIdx);
        viis.stepCount = variable->GetAvailableStepsCount();
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            viis.startStep = 0;
        }
        else
        {
            viis.startStep = m_StartStepInCurrentFile;
        }
        viis.endStep = viis.stepCount + viis.startStep - 1;
        VarInternalInfo vii(variable->m_Name, 1);
        vii.info.push_back(viis);

        // std::cout << "New variable " << v.m_Name << " from engine " << engineIdx
        // << " with shape " << helper::DimsToString(v.m_Shape) << "\n";
        m_VarInternalInfo.emplace(variable->m_Name, vii);
        return v;
    }
}

template <class T>
inline Attribute<T> TimeSeriesReader::DuplicateAttribute(Attribute<T> *attribute, IO &io)
{
    auto att = io.InquireAttribute<T>(attribute->m_Name);
    if (att != nullptr)
    {
        if (att->m_Type != attribute->m_Type)
        {
            helper::Throw<std::invalid_argument>(
                "Engine", "TimeSeriesReader", "DuplicateAttribute",
                "Attribute type mismatch in separate files, existing attribute type is " +
                    ToString(att->m_Type) + ", new type is " + ToString(attribute->m_Type));
        }

        if (att->m_AllowModification)
        {
            att->m_IsSingleValue = attribute->m_IsSingleValue;
            att->m_DataSingleValue = attribute->m_DataSingleValue;
            att->m_DataArray = attribute->m_DataArray;
            att->m_Elements = attribute->m_Elements;
        }
        return *att;
    }
    else
    {
        if (attribute->m_IsSingleValue)
        {
            auto &a = io.DefineAttribute<T>(attribute->m_Name, attribute->m_DataSingleValue, "",
                                            "/", attribute->m_AllowModification);
            return a;
        }
        auto &a =
            io.DefineAttribute<T>(attribute->m_Name, attribute->m_DataArray.data(),
                                  attribute->m_Elements, "", "/", attribute->m_AllowModification);
        return a;
    }
}

template <>
inline void TimeSeriesReader::GetCommon(Variable<std::string> &variable, std::string *data,
                                        adios2::Mode mode)
{
    // variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "TimeSeries Reader " << m_ReaderRank << "     GetSync( string variable "
                  << variable.m_Name << ")\n";
    }
}

template <class T>
inline void TimeSeriesReader::GetCommon(Variable<T> &variable, T *data, adios2::Mode mode)
{
    // variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "TimeSeries Reader " << m_ReaderRank << "     GetSync(" << variable.m_Name
                  << ")\n";
    }
    auto it = m_VarInternalInfo.find(variable.m_Name);
    auto vii = it->second;

    size_t startEngineIdx = FindStep(vii, variable.m_StepsStart);
    size_t endEngineIdx = FindStep(vii, variable.m_StepsStart + variable.m_StepsCount - 1);
    size_t nElemsPerStep = helper::GetTotalSize(variable.Count());
    /* std::cout << "TimeSeries Reader " << m_ReaderRank << "     GetSync(" << variable.m_Name
              << ")  startEngineIdx = " << startEngineIdx << " endEngineIdx = " << endEngineIdx
              << " nElemsPerStep = " << nElemsPerStep << "\n";*/
    size_t stepFrom = variable.m_StepsStart;
    size_t remaniningSteps = variable.m_StepsCount;
    for (size_t engineIdx = startEngineIdx; engineIdx <= endEngineIdx; ++engineIdx)
    {
        size_t engineStepStart = stepFrom - vii.info[engineIdx].startStep;
        auto viis = vii.info[engineIdx];
        Variable<T> *v = reinterpret_cast<Variable<T> *>(viis.originalVar);
        Engine *e = m_Engines[viis.engineIdx];
        size_t engineStepCount =
            (viis.stepCount > remaniningSteps ? remaniningSteps : viis.stepCount);
        if (engineStepCount > vii.info[engineIdx].stepCount - engineStepStart)
        {
            engineStepCount = vii.info[engineIdx].stepCount - engineStepStart;
        }

        /* std::cout << "    engineIdx = " << engineIdx << " step start = " << engineStepStart
                  << " step count = " << engineStepCount
                  << " data offset = " << ((stepFrom - variable.m_StepsStart) * nElemsPerStep)
                  << "\n";*/
        T *engineData = data + ((stepFrom - variable.m_StepsStart) * nElemsPerStep);
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            v->SetStepSelection(Box<size_t>(engineStepStart, engineStepCount));
        }
        if (!v->m_SingleValue)
        {
            if (v->m_SelectionType == SelectionType::BoundingBox)
            {
                v->SetSelection(Box<Dims>(variable.m_Start, variable.m_Count));
            }
            else if (v->m_SelectionType == SelectionType::WriteBlock)
            {
                v->SetBlockSelection(variable.m_BlockID);
            }
        }
        e->Get(*v, engineData, mode);
        stepFrom += engineStepCount;
        remaniningSteps -= engineStepCount;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_TIMESERIESREADER_TCC_
