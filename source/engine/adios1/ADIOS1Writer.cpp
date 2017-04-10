/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Writer.cpp
 * Class to write files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: pnb
 */

#include "engine/adios1/ADIOS1Writer.h"
#include "ADIOS.h"
#include "functions/adiosFunctions.h"

extern int adios_verbose_level;
extern int adios_errno;

namespace adios
{

ADIOS1Writer::ADIOS1Writer(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "ADIOS1Writer", name, accessMode, mpiComm, method,
         " ADIOS1Writer constructor (or call to ADIOS Open).\n"),
  m_groupname{method.m_Name.c_str()}, m_filename{name.c_str()}, m_comm{mpiComm}
{
    Init();
    adios_open(&m_adios_file, m_groupname, m_filename, accessMode.c_str(),
               m_comm);
    if (adios_errno == err_no_error)
    {
        m_IsFileOpen = true;
    }
}

ADIOS1Writer::~ADIOS1Writer()
{
    if (m_IsFileOpen)
    {
        adios_close(m_adios_file);
        m_IsFileOpen = false;
    }
}

void ADIOS1Writer::Init()
{
    if (!m_initialized)
    {
        adios_init_noxml(m_comm);
        m_initialized = true;
    }
    adios_declare_group(&m_adios_group, m_groupname, "", adios_stat_default);

    InitParameters();
    InitTransports();
}

bool ADIOS1Writer::ReOpenAsNeeded()
{
    if (!m_IsFileOpen)
    {
        adios_open(&m_adios_file, m_groupname, m_filename, "a", m_comm);
        if (adios_errno == err_no_error)
        {
            m_IsFileOpen = true;
            adios_delete_vardefs(m_adios_group);
        }
    }
    return m_IsFileOpen;
}

void ADIOS1Writer::DefineVariable(std::string name, bool isScalar,
                                  enum ADIOS_DATATYPES vartype,
                                  std::string ldims, std::string gdims,
                                  std::string offs)
{
    if (isScalar)
    {
        adios_define_var(m_adios_group, name.c_str(), "", vartype, "", "", "");
    }
    else
    {
        adios_define_var(m_adios_group, name.c_str(), "", vartype,
                         ldims.c_str(), gdims.c_str(), offs.c_str());
    }
}

void ADIOS1Writer::WriteVariable(std::string name, bool isScalar,
                                 enum ADIOS_DATATYPES vartype,
                                 std::string ldims, std::string gdims,
                                 std::string offs, const void *values)
{
    if (ReOpenAsNeeded())
    {
        DefineVariable(name, isScalar, vartype, ldims, gdims, offs);
        adios_write(m_adios_file, name.c_str(), values);
    }
}

void ADIOS1Writer::Write(Variable<char> &variable, const char *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_byte,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<unsigned char> &variable,
                         const unsigned char *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_unsigned_byte,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<short> &variable, const short *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_short,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<unsigned short> &variable,
                         const unsigned short *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_unsigned_short,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<int> &variable, const int *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_integer,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<unsigned int> &variable,
                         const unsigned int *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_unsigned_integer,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<long int> &variable, const long int *values)
{
    enum ADIOS_DATATYPES type =
        adios_integer; // long int is usually 4 bytes which is adios_integer
    if (sizeof(long int) == 8)
    {
        type = adios_long;
    }
    WriteVariable(variable.m_Name, variable.m_IsScalar, type,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<unsigned long int> &variable,
                         const unsigned long int *values)
{
    enum ADIOS_DATATYPES type =
        adios_unsigned_integer; // long int is usually 4 bytes
    if (sizeof(long int) == 8)
    {
        type = adios_unsigned_long;
    }
    WriteVariable(variable.m_Name, variable.m_IsScalar, type,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<long long int> &variable,
                         const long long int *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_long,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<unsigned long long int> &variable,
                         const unsigned long long int *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_unsigned_long,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<float> &variable, const float *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_real,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<double> &variable, const double *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_double,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<long double> &variable,
                         const long double *values)
{
    /* TODO: This is faulty: adios_long_double expects 16 bytes per elements,
     * but
     * long double is compiler dependent */
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_long_double,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<std::complex<float>> &variable,
                         const std::complex<float> *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_complex,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<std::complex<double>> &variable,
                         const std::complex<double> *values)
{
    WriteVariable(variable.m_Name, variable.m_IsScalar, adios_double_complex,
                  DimsToCSV(variable.m_LocalDimensions),
                  DimsToCSV(variable.m_GlobalDimensions),
                  DimsToCSV(variable.m_Offsets), values);
}

void ADIOS1Writer::Write(Variable<std::complex<long double>> &variable,
                         const std::complex<long double> *values)
{
    throw std::invalid_argument(
        "ERROR: Adios 1.x does not support complex<long "
        "double> type, so it cannot write variable " +
        variable.m_Name + "\n");
}

void ADIOS1Writer::Write(VariableCompound &variable, const void *values)
{
    throw std::invalid_argument("ERROR: Adios 1.x does not support compound "
                                "types, so it cannot write variable " +
                                variable.m_Name + "\n");
}

// String version
void ADIOS1Writer::Write(const std::string &variableName, const char *values)
{
    Write(m_ADIOS.GetVariable<char>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const unsigned char *values)
{
    Write(m_ADIOS.GetVariable<unsigned char>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName, const short *values)
{
    Write(m_ADIOS.GetVariable<short>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const unsigned short *values)
{
    Write(m_ADIOS.GetVariable<unsigned short>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName, const int *values)
{
    Write(m_ADIOS.GetVariable<int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const unsigned int *values)
{
    Write(m_ADIOS.GetVariable<unsigned int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const long int *values)
{
    Write(m_ADIOS.GetVariable<long int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const unsigned long int *values)
{
    Write(m_ADIOS.GetVariable<unsigned long int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const long long int *values)
{
    Write(m_ADIOS.GetVariable<long long int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const unsigned long long int *values)
{
    Write(m_ADIOS.GetVariable<unsigned long long int>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName, const float *values)
{
    Write(m_ADIOS.GetVariable<float>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName, const double *values)
{
    Write(m_ADIOS.GetVariable<double>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const long double *values)
{
    Write(m_ADIOS.GetVariable<long double>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const std::complex<float> *values)
{
    Write(m_ADIOS.GetVariable<std::complex<float>>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const std::complex<double> *values)
{
    Write(m_ADIOS.GetVariable<std::complex<double>>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const std::complex<long double> *values)
{
    Write(m_ADIOS.GetVariable<std::complex<long double>>(variableName), values);
}

void ADIOS1Writer::Write(const std::string &variableName,
                         const void *values) // Compound type
{
    throw std::invalid_argument("ERROR: Adios 1.x does not support compound "
                                "types, so it cannot write variable " +
                                variableName + "\n");
}

void ADIOS1Writer::Advance(const float /*timeout_sec*/)
{
    if (m_IsFileOpen)
    {
        adios_close(m_adios_file);
        m_IsFileOpen = false;
    }
}

void ADIOS1Writer::Close(const int transportIndex)
{
    if (m_IsFileOpen)
    {
        adios_close(m_adios_file);
        m_IsFileOpen = false;
    }
}

// PRIVATE FUNCTIONS
void ADIOS1Writer::InitParameters()
{
    auto itMaxBufferSize = m_Method.m_Parameters.find("max_size_MB");
    if (itMaxBufferSize != m_Method.m_Parameters.end())
    {
        adios_set_max_buffer_size(std::stoul(itMaxBufferSize->second));
    }

    auto itVerbosity = m_Method.m_Parameters.find("verbose");
    if (itVerbosity != m_Method.m_Parameters.end())
    {
        int verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode == true)
        {
            if (verbosity < 0 || verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
        adios_verbose_level = verbosity;
    }
}

void ADIOS1Writer::InitTransports()
{
    if (m_DebugMode == true)
    {
        if (TransportNamesUniqueness() == false)
        {
            throw std::invalid_argument(
                "ERROR: two transports of the same kind (e.g file IO) "
                "can't have the same name, modify with name= in Method "
                "AddTransport\n");
        }
    }

    for (const auto &parameters : m_Method.m_TransportParameters)
    {
        auto itTransport = parameters.find("transport");

        if (itTransport->second == "file" || itTransport->second == "File")
        {
            auto itLibrary = parameters.find("library");
            if (itLibrary == parameters.end() ||
                itLibrary->second == "POSIX") // use default POSIX
            {
                adios_select_method(m_adios_group, "POSIX", "", "");
            }
            else if (itLibrary->second == "MPI_File" ||
                     itLibrary->second == "MPI-IO")
            {
                adios_select_method(m_adios_group, "MPI", "", "");
            }
            else
            {
                if (m_DebugMode == true)
                    throw std::invalid_argument(
                        "ERROR: file transport library " + itLibrary->second +
                        " not supported, in " + m_Name + m_EndMessage);
            }
        }
        else
        {
            if (m_DebugMode == true)
                throw std::invalid_argument(
                    "ERROR: transport " + itTransport->second +
                    " (you mean File?) not supported, in " + m_Name +
                    m_EndMessage);
        }
    }
}

} // end namespace adios
