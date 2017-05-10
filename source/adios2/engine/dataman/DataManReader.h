/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.h
 *
 *  Created on: Feb 21, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_

#include <iostream> //std::cout << Needs to go

#include <DataMan.h>

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios
{

class DataManReader : public Engine
{

public:
    /**
     * Constructor for dataman engine Reader for WAN communications
     * @param adios
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    using json = nlohmann::json;
    DataManReader(IO &io, const std::string &name, const OpenMode openMode,
                  MPI_Comm mpiComm);

    virtual ~DataManReader() = default;

    /**
     * Set callback function from user application
     * @param callback function (get) provided by the user to be applied in
     * DataMan
     */
    void SetCallBack(std::function<void(const void *, std::string, std::string,
                                        std::string, Dims)>
                         callback);

    /**
     * Not implemented
     * @param name
     * @param readIn
     * @return
     */
    VariableCompound *InquireVariableCompound(const std::string &name,
                                              const bool readIn = true);

    void Close(const int transportIndex = -1);

private:
    bool m_DoRealTime = false;
    DataMan m_Man;
    std::function<void(const void *, std::string, std::string, std::string,
                       Dims)>
        m_CallBack; ///< call back function

    void Init();

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string name,
                                       const bool readIn)
    {
        int rank = 0;
        MPI_Comm_rank(m_MPIComm, &rank);

        std::cout << "I am hooked to the DataMan library\n";
        std::cout << "Hello DatamanReader from rank " << rank << "\n";
        std::cout << "Trying to read variable " << name
                  << " from one of the variables coming from a WAN transport\n";

        // here read variable metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        return nullptr; // on failure
    }
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_ */
