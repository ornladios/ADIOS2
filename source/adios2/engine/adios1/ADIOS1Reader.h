/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.h
 * Class to read files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: pnb
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_

#include <iostream> //this must go away

#include "adios2/ADIOS.h"
#include "adios2/ADIOSConfig.h"
#include "adios2/capsule/heap/STLVector.h"
#include "adios2/core/Engine.h"
#include "adios2/core/adiosFunctions.h"

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios_read_v2.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

namespace adios
{

class ADIOS1Reader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    ADIOS1Reader(ADIOS &adios, const std::string &name,
                 const std::string accessMode, MPI_Comm mpiComm,
                 const Method &method);

    ~ADIOS1Reader();

    Variable<void> *InquireVariable(const std::string &name,
                                    const bool readIn = true);
    Variable<char> *InquireVariableChar(const std::string &name,
                                        const bool readIn = true);
    Variable<unsigned char> *InquireVariableUChar(const std::string &name,
                                                  const bool readIn = true);
    Variable<short> *InquireVariableShort(const std::string &name,
                                          const bool readIn = true);
    Variable<unsigned short> *InquireVariableUShort(const std::string &name,
                                                    const bool readIn = true);
    Variable<int> *InquireVariableInt(const std::string &name,
                                      const bool readIn = true);
    Variable<unsigned int> *InquireVariableUInt(const std::string &name,
                                                const bool readIn = true);
    Variable<long int> *InquireVariableLInt(const std::string &name,
                                            const bool readIn = true);
    Variable<unsigned long int> *InquireVariableULInt(const std::string &name,
                                                      const bool readIn = true);
    Variable<long long int> *InquireVariableLLInt(const std::string &name,
                                                  const bool readIn = true);
    Variable<unsigned long long int> *
    InquireVariableULLInt(const std::string &name, const bool readIn = true);
    Variable<float> *InquireVariableFloat(const std::string &name,
                                          const bool readIn = true);
    Variable<double> *InquireVariableDouble(const std::string &name,
                                            const bool readIn = true);
    Variable<long double> *InquireVariableLDouble(const std::string &name,
                                                  const bool readIn = true);
    Variable<std::complex<float>> *
    InquireVariableCFloat(const std::string &name, const bool readIn = true);
    Variable<std::complex<double>> *
    InquireVariableCDouble(const std::string &name, const bool readIn = true);
    Variable<std::complex<long double>> *
    InquireVariableCLDouble(const std::string &name, const bool readIn = true);

    /**
     * Not implemented
     * @param name
     * @param readIn
     * @return
     */
    VariableCompound *InquireVariableCompound(const std::string &name,
                                              const bool readIn = true);

    void ScheduleRead(Variable<char> &variable, char *values);
    void ScheduleRead(Variable<unsigned char> &variable, unsigned char *values);
    void ScheduleRead(Variable<short> &variable, short *values);
    void ScheduleRead(Variable<unsigned short> &variable,
                      unsigned short *values);
    void ScheduleRead(Variable<int> &variable, int *values);
    void ScheduleRead(Variable<unsigned int> &variable, unsigned int *values);
    void ScheduleRead(Variable<long int> &variable, long int *values);
    void ScheduleRead(Variable<unsigned long int> &variable,
                      unsigned long int *values);
    void ScheduleRead(Variable<long long int> &variable, long long int *values);
    void ScheduleRead(Variable<unsigned long long int> &variable,
                      unsigned long long int *values);
    void ScheduleRead(Variable<float> &variable, float *values);
    void ScheduleRead(Variable<double> &variable, double *values);
    void ScheduleRead(Variable<long double> &variable, long double *values);
    void ScheduleRead(Variable<std::complex<float>> &variable,
                      std::complex<float> *values);
    void ScheduleRead(Variable<std::complex<double>> &variable,
                      std::complex<double> *values);
    void ScheduleRead(Variable<std::complex<long double>> &variable,
                      std::complex<long double> *values);

    void ScheduleRead(const std::string &variableName, char *values);
    void ScheduleRead(const std::string &variableName, unsigned char *values);
    void ScheduleRead(const std::string &variableName, short *values);
    void ScheduleRead(const std::string &variableName, unsigned short *values);
    void ScheduleRead(const std::string &variableName, int *values);
    void ScheduleRead(const std::string &variableName, unsigned int *values);
    void ScheduleRead(const std::string &variableName, long int *values);
    void ScheduleRead(const std::string &variableName,
                      unsigned long int *values);
    void ScheduleRead(const std::string &variableName, long long int *values);
    void ScheduleRead(const std::string &variableName,
                      unsigned long long int *values);
    void ScheduleRead(const std::string &variableName, float *values);
    void ScheduleRead(const std::string &variableName, double *values);
    void ScheduleRead(const std::string &variableName, long double *values);
    void ScheduleRead(const std::string &variableName,
                      std::complex<float> *values);
    void ScheduleRead(const std::string &variableName,
                      std::complex<double> *values);
    void ScheduleRead(const std::string &variableName,
                      std::complex<long double> *values);

    void PerformReads(PerformReadMode mode);

    void Release();
    void Advance(const float timeout_sec = 0.0);
    void Advance(AdvanceMode mode, const float timeout_sec = 0.0);
    void
    AdvanceAsync(AdvanceMode mode,
                 std::function<void(std::shared_ptr<adios::Engine>)> callback);

    void Close(const int transportIndex = -1);

private:
    ADIOS_FILE *m_fh = nullptr; ///< ADIOS1 file handler
    bool m_OpenAsFile = false;

    void Init(); ///< called from constructor, gets the selected ADIOS1
                 /// transport method from settings
    void InitParameters();
    void InitTransports();

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string &name,
                                       const bool readIn)
    {
        // here read variable metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        ADIOS_VARINFO *vi = adios_inq_var(m_fh, name.c_str());
        adios::Variable<T> *var = nullptr;
        if (vi != nullptr)
        {
            CheckADIOS1TypeCompatibility(name, GetType<T>(),
                                         vi->type); // throws

            if (vi->ndim > 0)
            {
                Dims gdims = Uint64ArrayToSizetVector(vi->ndim, vi->dims);

                bool joinedread = false;
                if (gdims[0] == JoinedDim)
                {
                    /* Joined Array */
                    adios_inq_var_blockinfo(m_fh, vi);
                    size_t joined_size = 0;
                    for (int i = 0; i < *vi->nblocks; i++)
                    {
                        joined_size += vi->blockinfo[i].count[0];
                    }
                    gdims[0] = joined_size;
                    joinedread = true;
                }

                if (!vi->global)
                {
                    /* Local array */
                    for (int j = 0; j < vi->ndim; ++j)
                    {
                        gdims[j] = IrregularDim;
                    }
                }
                else
                {
                    /* Check if dimensions change in time */
                    for (int step = 1; step < vi->nsteps; ++step)
                    {
                        Dims dims =
                            gdims; // GetGlobalDimsAtStep(vi, step, joinedread);
                        for (int j = 0; j < vi->ndim; ++j)
                        {
                            if (dims[j] != gdims[j])
                                gdims[j] = IrregularDim;
                        }
                    }
                }
                var = &m_ADIOS.DefineArray<T>(name, gdims);
                if (joinedread)
                    var->SetReadAsJoinedArray();
            }
            else /* Scalars */
            {
                /* scalar variable but global value or local value*/
                std::string aname = name + "/ReadAsArray";
                bool isLocalValue = false;
                for (int i = 0; i < vi->nattrs; ++i)
                {
                    if (!strcmp(m_fh->attr_namelist[vi->attr_ids[i]],
                                aname.c_str()))
                    {
                        isLocalValue = true;
                    }
                }
                if (isLocalValue)
                {
                    /* Local Value */
                    bool changingDims = false;
                    for (int step = 1; step < vi->nsteps; ++step)
                    {
                        if (vi->nblocks[step] != vi->nblocks[0])
                            changingDims = true;
                    }
                    if (changingDims)
                    {
                        var = &m_ADIOS.DefineVariable<T>(name, {IrregularDim});
                    }
                    else
                    {
                        var = &m_ADIOS.DefineVariable<T>(
                            name, {(unsigned int)vi->nblocks[0]});
                    }
                    var->SetReadAsLocalValue();
                }
                else
                {
                    /* Global value: store only one value */
                    var = &m_ADIOS.DefineVariable<T>(name);
                    if (var)
                    {
                        var->m_Data = std::vector<T>(1);
                        var->m_Data[0] = *static_cast<T *>(vi->value);
                    }
                }
            }
            var->SetNSteps(vi->nsteps);
            adios_free_varinfo(vi);
        }
        return var;
    }

    void ScheduleReadCommon(const std::string &name, const Dims &offs,
                            const Dims &ldims, const int fromStep,
                            const int nSteps, const bool readAsLocalValue,
                            const bool readAsJoinedArray, void *data);

    void ReadJoinedArray(const std::string &name, const Dims &offs,
                         const Dims &ldims, const int fromStep,
                         const int nSteps, void *data);

    bool CheckADIOS1TypeCompatibility(const std::string &name,
                                      std::string adios2Type,
                                      enum ADIOS_DATATYPES adios1Type);

    enum ADIOS_READ_METHOD m_ReadMethod = ADIOS_READ_METHOD_BP;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_ */
