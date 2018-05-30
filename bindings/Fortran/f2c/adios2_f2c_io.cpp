/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_io.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_io.h"

#include <cstddef>  //std::size_t
#include <iostream> //std::cerr
#include <stdexcept>
#include <vector>

void FC_GLOBAL(adios2_set_engine_f2c,
               ADIOS2_SET_ENGINE_F2C)(adios2_io **io, const char *engine_type,
                                      int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_engine(*io, engine_type);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_engine: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_parameter_f2c,
               ADIOS2_SET_PARAMETER_F2C)(adios2_io **io, const char *key,
                                         const char *value, int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_parameter(*io, key, value);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_parameter: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_add_transport_f2c,
               ADIOS2_ADD_TRANSPORT_F2C)(int *transport_index, adios2_io **io,
                                         const char *transport_type, int *ierr)
{
    *ierr = 0;
    *transport_index = -1;

    try
    {
        *transport_index =
            static_cast<int>(adios2_add_transport(*io, transport_type));

        if (*transport_index == -1)
        {
            throw std::invalid_argument("ERROR: transport_index handler not "
                                        "set, in call to "
                                        "adios2_add_transport\n");
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 add_transport: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_transport_parameter_f2c,
               ADIOS2_SET_TRANSPORT_PARAMETER_F2C)(adios2_io **io,
                                                   const int *transport_index,
                                                   const char *key,
                                                   const char *value, int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_transport_parameter(
            *io, static_cast<unsigned int>(*transport_index), key, value);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_transport_parameter: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_define_global_variable_f2c,
               ADIOS2_DEFINE_GLOBAL_VARIABLE_F2C)(adios2_variable **variable,
                                                  adios2_io **io,
                                                  const char *name,
                                                  const int *type, int *ierr)
{
    *ierr = 0;
    try
    {
        *variable = adios2_define_variable(
            *io, name, static_cast<adios2_type>(*type), 0, nullptr, nullptr,
            nullptr, adios2_constant_dims_true, nullptr);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 define_variable " << name << ": " << e.what()
                  << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_define_variable_f2c, ADIOS2_DEFINE_VARIABLE_F2C)(
    adios2_variable **variable, adios2_io **io, const char *name,
    const int *type, const int *ndims, const int64_t *shape,
    const int64_t *start, const int64_t *count, const int *constant_dims,
    int *ierr)
{
    auto lf_IntToSizeT = [](const int64_t *dimensions, const int size,
                            std::vector<std::size_t> &output) {

        output.resize(size);

        for (unsigned int d = 0; d < size; ++d)
        {
            if (dimensions[d] < 0)
            {
                throw std::invalid_argument("ERROR: negative shape, start, or "
                                            "count dimension in Fortran\n");
            }
            output[d] = dimensions[d];
        }
    };

    *ierr = 0;

    try
    {
        if (*ndims <= 0)
        {
            throw std::invalid_argument("ERROR: negative ndims in Fortran ");
        }

        // Check for local variables
        if (shape[0] == -1)
        {
            if (start[0] != -1)
            {
                throw std::invalid_argument("ERROR: shape and start must be "
                                            "adios2_null_dims when declaring "
                                            "local variables in Fortran ");
            }

            std::vector<std::size_t> countV;
            lf_IntToSizeT(count, *ndims, countV);

            *variable = adios2_define_variable(
                *io, name, static_cast<adios2_type>(*type), *ndims, nullptr,
                nullptr, countV.data(),
                static_cast<adios2_constant_dims>(*constant_dims), nullptr);
            return;
        }

        std::vector<std::size_t> shapeV, startV, countV;
        lf_IntToSizeT(shape, *ndims, shapeV);
        lf_IntToSizeT(start, *ndims, startV);
        lf_IntToSizeT(count, *ndims, countV);

        *variable = adios2_define_variable(
            *io, name, static_cast<adios2_type>(*type),
            static_cast<size_t>(*ndims), shapeV.data(), startV.data(),
            countV.data(), static_cast<adios2_constant_dims>(*constant_dims),
            nullptr);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 define_variable " << name << ": " << e.what()
                  << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_inquire_variable_f2c,
               ADIOS2_INQUIRE_VARIABLE_F2C)(adios2_variable **variable,
                                            adios2_io **io,
                                            const char *variable_name,
                                            int *ierr)
{
    *ierr = 0;
    try
    {
        *variable = adios2_inquire_variable(*io, variable_name);
        if (*variable == nullptr)
        {
            *ierr = 1;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 inquire_variable: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_remove_variable_f2c,
               ADIOS2_REMOVE_VARIABLE_F2C)(adios2_io **io, const char *name,
                                           int *ierr)
{
    *ierr = 0;
    try
    {
        *ierr = adios2_remove_variable(*io, name);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 remove_variable: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_remove_all_variables_f2c,
               ADIOS2_REMOVE_ALL_VARIABLES_F2C)(adios2_io **io, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_remove_all_variables(*io);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 remove_all_variables: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_define_attribute_f2c,
               ADIOS2_DEFINE_ATTRIBUTE_F2C)(adios2_attribute **attribute,
                                            adios2_io **io, const char *name,
                                            const int *type, const void *data,
                                            const int *elements, int *ierr)
{
    *ierr = 0;
    try
    {
        *attribute =
            adios2_define_attribute(*io, name, static_cast<adios2_type>(*type),
                                    data, static_cast<std::size_t>(*elements));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 define_attribute: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_remove_attribute_f2c,
               ADIOS2_REMOVE_ATTRIBUTE_F2C)(adios2_io **io, const char *name,
                                            int *ierr)
{
    *ierr = 0;
    try
    {
        *ierr = adios2_remove_attribute(*io, name);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 remove_attribute: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_remove_all_attributes_f2c,
               ADIOS2_REMOVE_ALL_ATTRIBUTES_F2C)(adios2_io **io, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_remove_all_attributes(*io);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 remove_all_attributes: " << e.what() << "\n";
        *ierr = -1;
    }
}

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_open_new_comm_f2c,
               ADIOS2_OPEN_NEW_COMM_F2C)(adios2_engine **engine, adios2_io **io,
                                         const char *name, const int *open_mode,
                                         MPI_Fint *comm, int *ierr)
{
    *ierr = 0;
    try
    {
        *engine = adios2_open_new_comm(*io, name,
                                       static_cast<adios2_mode>(*open_mode),
                                       MPI_Comm_f2c(*comm));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 open with new MPI_Comm: " << e.what() << "\n";
        *ierr = -1;
    }
}
#endif

void FC_GLOBAL(adios2_open_f2c,
               ADIOS2_OPEN_F2C)(adios2_engine **engine, adios2_io **io,
                                const char *name, const int *open_mode,
                                int *ierr)
{
    *ierr = 0;
    try
    {
        *engine = adios2_open(*io, name, static_cast<adios2_mode>(*open_mode));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 open: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_flush_all_engines_f2c,
               ADIOS2_FLUSH_ALL_ENGINES_F2C)(adios2_io **io, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_flush_all_engines(*io);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 flush_all_engines: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_io_engine_type_f2c,
               ADIOS2_IO_ENGINE_TYPE_F2C)(const adios2_io **io,
                                          char engine_type[32], int *length,
                                          int *ierr)
{
    *ierr = 0;
    try
    {
        std::size_t lengthC = 0;
        const char *nameC = adios2_io_engine_type(*io, &lengthC);

        if (nameC == nullptr)
        {
            throw std::runtime_error(
                "ERROR: null pointer in adios2 io engine type\n");
        }

        for (std::size_t i = 0; i < lengthC; ++i)
        {
            engine_type[i] = nameC[i];
        }

        *length = static_cast<int>(lengthC);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 io_engine_type: " << e.what() << "\n";
        *ierr = -1;
    }
}
