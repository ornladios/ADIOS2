/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TimeSeries.h
 * A meta engine to process a series of files using existing engines.
 * File format:
 *   filename1
 *   filename2
 *   filename3
 *   --end--
 *
 * --end-- signals that there is no more files coming to the stream
 *
 *  Created on: Apr 30, 2025
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_TIMESERIESREADER_H_
#define ADIOS2_ENGINE_TIMESERIESREADER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosYAML.h"

#include <deque>
#include <fstream>

namespace adios2
{
namespace core
{
namespace engine
{

class TimeSeriesReader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     * @param hostLanguage
     */
    TimeSeriesReader(IO &adios, const std::string &name, const Mode mode, helper::Comm comm);

    ~TimeSeriesReader();
    StepStatus BeginStep(StepMode mode = StepMode::Read, const float timeoutSeconds = -1.0) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

    MinVarInfo *MinBlocksInfo(const VariableBase &, const size_t Step) const;
    bool VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const;
    bool VariableMinMax(const VariableBase &, const size_t Step, MinMaxStruct &MinMax);
    std::string VariableExprStr(const VariableBase &Var);

private:
    int m_Verbosity = 0;
    int m_ReaderRank;         // my rank in the readers' comm
    std::string m_ATSFileDir; // directory of the ATS file, for relative paths
    helper::TimeSeriesList m_TimeSeriesList;

    int m_CurrentStep = 0;
    size_t m_StepsCount = 0;
    int m_StartStepInCurrentFile = 0; // separate record for streamin mode
    bool m_FirstStep = true;
    bool m_BetweenStepPairs = false;
    bool m_NeedPerformGets = false;

    std::vector<adios2::core::IO *> m_IOs;
    std::vector<adios2::core::Engine *> m_Engines;

    struct VarInternalInfoSingle
    {
        void *originalVar; // Variable<T> in the actual IO
        size_t ioIdx;      // actual IO in m_IOs
        size_t engineIdx;  // actual engine in m_Engines
        size_t startStep;  // start step in the engine
        size_t endStep;    // end step in the engine
        size_t stepCount;  // number of steps in the engine
        VarInternalInfoSingle(void *p, size_t i, size_t e) : originalVar(p), ioIdx(i), engineIdx(e)
        {
        }
    };

    struct VarInternalInfo
    {
        const std::string name;                  // variable name
        size_t nEngines;                         // number of engines this variable
        std::vector<VarInternalInfoSingle> info; // Per-engine variable information
        VarInternalInfo(const std::string &name, size_t n) : name(name), nEngines(n)
        {
            info.reserve(n);
        }
    };
    std::unordered_map<std::string, VarInternalInfo> m_VarInternalInfo;

    /**
     * Create a new variable with name `name` in `io`
     * based on an existing variable.
     */
    template <class T>
    Variable<T> DuplicateVariable(Variable<T> *variable, IO &io, size_t ioIdx, size_t engineIdx,
                                  T min, T max);

    /**
     * Create a new attribute with name `name` in `io`
     * based on an existing attribute.
     */
    template <class T>
    Attribute<T> DuplicateAttribute(Attribute<T> *attribute, IO &io);

    size_t FindStep(const VarInternalInfo &vii, const size_t step) const;

    void Init() final; ///< called from constructor, gets the selected TimeSeries
                       /// transport method from settings
    void InitParameters() final;
    void InitTransports() final;
    void ProcessIO(adios2::core::IO &io, adios2::core::Engine &e);
    void InitFile(const helper::TimeSeriesEntry &tse,
                  bool process); // open one file and process its variables and attributes
    bool CheckForFiles();        // read (new) entries in ATS file until --end--

#define declare_type(T)                                                                            \
    void DoGetSync(Variable<T> &, T *) final;                                                      \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1);

    size_t DoSteps() const final;

#define declare_type(T)                                                                            \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>> DoAllStepsBlocksInfo(              \
        const Variable<T> &variable) const final;                                                  \
                                                                                                   \
    std::vector<std::vector<typename Variable<T>::BPInfo>> DoAllRelativeStepsBlocksInfo(           \
        const Variable<T> &) const final;                                                          \
                                                                                                   \
    std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(const Variable<T> &variable,            \
                                                           const size_t step) const final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Called if destructor is called on an open engine.  Should warn or take
     * any non-complex measure that might help recover.
     */
    void DestructorClose(bool Verbose) noexcept final { DoClose(); };

    template <class T>
    void GetCommon(Variable<T> &variable, T *data, adios2::Mode mode);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TIMESERIESREADER_H_ */
