/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataSpacesReader.h
 *
 *  Created on: Dec 5, 2018
 *      Author: Pradeep Subedi
 *      		pradee.subedi@rutgers.edu
 */

#ifndef ADIOS2_ENGINE_DATASPACES_DATASPACESREADER_H_
#define ADIOS2_ENGINE_DATASPACES_DATASPACESREADER_H_

#include "adios2/ADIOSConfig.h"
#include "mpi.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/dataspaces/ds.h"

namespace adios2
{
namespace core
{
namespace engine
{

class DataSpacesReader : public Engine
{

public:
    DataSpacesReader(IO &adios, const std::string &name, const Mode openMode,
    		MPI_Comm mpiComm);

    ~DataSpacesReader();
    StepStatus BeginStep();
    StepStatus BeginStep(
        StepMode mode,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    size_t CurrentStep() const final;
    void EndStep() final;

    void PerformGets() final;
    void Flush(const int transportIndex = -1) final;

private:
    DsData m_data;
    std::string f_Name;
    int latestStep;
    int nVars;
    int m_CurrentStep;
    const std::map<int, std::string>ds_to_varType = {
                {1, "char"},
                {2, "signed char"},
                {3, "unsigned char"},
                {4, "short"},
                {5, "unsigned short"},
                {6, "int"},
                {7, "unsigned int"},
                {8, "long int"},
                {9, "long long int"},
                {10, "unsigned long int"},
                {11, "unsigned long long int"},
                {12, "float"},
                {13, "double"},
                {14, "long double"},
                {15, "complex float"},
                {16, "complex double"},
				{17, "string"},
        };

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
        void GetDeferredCommon(Variable<T> &variable, T *data);

    template <class T>
    void AddVar(core::IO &io, std::string const &name, Dims shape);

    //template <class T>
      //  void DspacesRead(Variable<T> &variable, T *data);

    template <class T>
    void ReadDsData(Variable<T> &variable, T *data, int version);

    std::vector<std::string> m_DeferredStack;

};

} // end namespace engine
} // end namespace core
} // end namespace adios2



#endif /* ADIOS2_ENGINE_DATASPACES_DATASPACESREADER_H_ */
