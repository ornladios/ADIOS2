/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Writer.h
 * Class to write files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: pnb
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

namespace adios
{

class ADIOS1Writer : public Engine
{

public:
    /**
     * Constructor for Writer writes in ADIOS 1.x BP format
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    ADIOS1Writer(ADIOS &adios, const std::string &name,
                 const std::string accessMode, MPI_Comm mpiComm,
                 const Method &method);

    ~ADIOS1Writer();

    void Write(Variable<char> &variable, const char *values);
    void Write(Variable<unsigned char> &variable, const unsigned char *values);
    void Write(Variable<short> &variable, const short *values);
    void Write(Variable<unsigned short> &variable,
               const unsigned short *values);
    void Write(Variable<int> &variable, const int *values);
    void Write(Variable<unsigned int> &variable, const unsigned int *values);
    void Write(Variable<long int> &variable, const long int *values);
    void Write(Variable<unsigned long int> &variable,
               const unsigned long int *values);
    void Write(Variable<long long int> &variable, const long long int *values);
    void Write(Variable<unsigned long long int> &variable,
               const unsigned long long int *values);
    void Write(Variable<float> &variable, const float *values);
    void Write(Variable<double> &variable, const double *values);
    void Write(Variable<long double> &variable, const long double *values);
    void Write(Variable<std::complex<float>> &variable,
               const std::complex<float> *values);
    void Write(Variable<std::complex<double>> &variable,
               const std::complex<double> *values);
    void Write(Variable<std::complex<long double>> &variable,
               const std::complex<long double> *values);
    void Write(VariableCompound &variable, const void *values);

    void Write(const std::string &variableName, const char *values);
    void Write(const std::string &variableName, const unsigned char *values);
    void Write(const std::string &variableName, const short *values);
    void Write(const std::string &variableName, const unsigned short *values);
    void Write(const std::string &variableName, const int *values);
    void Write(const std::string &variableName, const unsigned int *values);
    void Write(const std::string &variableName, const long int *values);
    void Write(const std::string &variableName,
               const unsigned long int *values);
    void Write(const std::string &variableName, const long long int *values);
    void Write(const std::string &variableName,
               const unsigned long long int *values);
    void Write(const std::string &variableName, const float *values);
    void Write(const std::string &variableName, const double *values);
    void Write(const std::string &variableName, const long double *values);
    void Write(const std::string &variableName,
               const std::complex<float> *values);
    void Write(const std::string &variableName,
               const std::complex<double> *values);
    void Write(const std::string &variableName,
               const std::complex<long double> *values);
    void Write(const std::string &variableName, const void *values);

    void Advance(const float timeout_sec = 0.);

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports, otherwise
     * it
     * closes a transport in m_Transport[transportIndex]. In debug mode the
     * latter
     * is bounds-checked.
     */
    void Close(const int transportIndex = -1);

private:
    const char *m_groupname; ///< ADIOS1 group name created from the method's
                             /// name. Must be a unique group name.
    const char *m_filename;  ///< Save file name from constructor for Advance()
                             /// when we re-open in ADIOS1
    MPI_Comm m_comm; ///< Save MPI communicator from constructor for Advance()
                     /// when we re-open in ADIOS1

    bool m_initialized = false; ///< set to true after calling adios_init()
    int64_t m_adios_file = 0;  ///< ADIOS1 file handler returned by adios_open()
    int64_t m_adios_group = 0; ///< ADIOS1 group pointer that holds the ADIOS1
                               /// variable definitions
    bool m_IsFileOpen = false;

    void Init();
    // these are unused yet, keeping here to see if we need them
    void InitParameters();
    void InitTransports();
    void InitProcessGroup();

    bool ReOpenAsNeeded(); // return true if file is open or reopened
    void DefineVariable(std::string name, VarClass varclass,
                        enum ADIOS_DATATYPES vartype, std::string ldims,
                        std::string gdims, std::string offs);
    void WriteVariable(std::string name, VarClass varclass,
                       enum ADIOS_DATATYPES vartype, std::string ldims,
                       std::string gdims, std::string offs, const void *values);
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_ */
