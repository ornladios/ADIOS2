/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Common.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#include "ADIOS1Common.h"
#include "ADIOS1Common.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //OpenModeToString, GetType

namespace adios2
{
namespace interop
{

ADIOS1Common::ADIOS1Common(const std::string &fileName, MPI_Comm mpiComm,
                           const bool debugMode)
: m_FileName(fileName), m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
}

ADIOS1Common::~ADIOS1Common() {}

std::string ADIOS1Common::DimsToCSVLocalAware(const Dims &dims)
{
    std::string dimsCSV;
    bool localVariable = false;

    for (const auto dim : dims)
    {
        dimsCSV += std::to_string(dim) + ",";
        if (dim == JoinedDim || dim == IrregularDim)
        {
            localVariable = true;
        }
    }

    if (!dimsCSV.empty())
    {
        dimsCSV.pop_back();
    }

    return dimsCSV;
}

static void CheckADIOS1Type(const std::string &name, std::string adios2Type,
                            std::string adios1Type)
{
    if (adios1Type != adios2Type)
    {
        throw std::invalid_argument(
            "Type mismatch. The expected ADIOS2 type <" + adios2Type +
            "> is not compatible with ADIOS1 type <" + adios1Type +
            "> of the requested variable '" + name + "'\n");
    }
}

bool ADIOS1Common::CheckADIOS1TypeCompatibility(const std::string &name,
                                                std::string adios2Type,
                                                enum ADIOS_DATATYPES adios1Type)
{
    bool compatible = false;
    switch (adios1Type)
    {
    case adios_unsigned_byte:
        CheckADIOS1Type(name, adios2Type, "unsigned char");
        break;
    case adios_unsigned_short:
        CheckADIOS1Type(name, adios2Type, "unsigned short");
        break;
    case adios_unsigned_integer:
        CheckADIOS1Type(name, adios2Type, "unsigned int");
        break;
    case adios_unsigned_long:
        CheckADIOS1Type(name, adios2Type, "unsigned long long int");
        break;

    case adios_byte:
        CheckADIOS1Type(name, adios2Type, "char");
        break;
    case adios_short:
        CheckADIOS1Type(name, adios2Type, "short");
        break;
    case adios_integer:
        CheckADIOS1Type(name, adios2Type, "int");
        break;
    case adios_long:
        CheckADIOS1Type(name, adios2Type, "long long int");
        break;

    case adios_real:
        CheckADIOS1Type(name, adios2Type, "float");
        break;
    case adios_double:
        CheckADIOS1Type(name, adios2Type, "double");
        break;
    case adios_long_double:
        CheckADIOS1Type(name, adios2Type, "long double");
        break;

    case adios_string:
        CheckADIOS1Type(name, adios2Type, "string");
        break;
    case adios_complex:
        CheckADIOS1Type(name, adios2Type, "float complex");
        break;
    case adios_double_complex:
        CheckADIOS1Type(name, adios2Type, "double complex");
        break;

    case adios_string_array:
        compatible = false;
        break;
    default:
        compatible = false;
    }
    return true;
}

} // end namespace interop
} // end namespace adios
