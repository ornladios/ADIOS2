/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataSpacesWriter.h
 *
 *  Created on: Dec 5, 2018
 *      Author: Pradeep Subedi
 *      		pradeep.subedi@rutgers.edu
 */

#ifndef ADIOS2_ENGINE_DATASPACES_DATASPACESWRITER_H_
#define ADIOS2_ENGINE_DATASPACES_DATASPACESWRITER_H_


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

class DataSpacesWriter : public Engine
{

public:
    /**
     * Constructor for DataSpaces writer engine,
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     */
    DataSpacesWriter(IO &io, const std::string &name, const Mode mode,
                MPI_Comm mpiComm);

   ~DataSpacesWriter();

    StepStatus BeginStep(
        StepMode mode,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    size_t CurrentStep() const final;
    void EndStep() final;

    void PerformPuts() final;
    void Flush(const int transportIndex = -1) final;


private:

    DsData m_data;
    std::string f_Name;
    int m_CurrentStep = -1; // steps start from 0
    std::vector<int> ndim_vector;
    std::vector<std::vector<uint64_t>> gdims_vector;
    std::vector<std::string> v_name_vector;
    std::vector<int> elemSize_vector;
    const std::map<std::string, int>varType_to_ds = {
    		{"char", 1},
			{"signed char", 2},
			{"unsigned char", 3},
			{"short", 4},
			{"unsigned short", 5},
			{"int", 6},
			{"unsigned int", 7},
			{"long int", 8},
			{"long long int", 9},
			{ "unsigned long int", 10},
			{"unsigned long long int", 11},
			{"float", 12},
			{"double", 13},
			{"long double", 14},
			{"complex float", 15},
			{"complex double", 16},
			{"string", 17},
    };

#define declare_type(T)                                                          \
    void DoPutSync(Variable<T> &variable, const T *values) final;              \
    void DoPutDeferred(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void DoPutSyncCommon(core::Variable<T> &variable, const T *values);
    void WriteVarInfo();

    void DoClose(const int transportIndex = -1) final;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2




#endif /* ADIOS2_ENGINE_DATASPACES_DATASPACESWRITER_H_ */
