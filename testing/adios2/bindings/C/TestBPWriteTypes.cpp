/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteTypes.c
 *
 *  Created on: Aug 9, 2017
 *      Author: Haocheng
 */

#include <adios2_c.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include <gtest/gtest.h>

#include "SmallTestData_c.h"

class BPWriteTypesCC : public ::testing::Test
{
public:
    BPWriteTypesCC() = default;
};

TEST_F(BPWriteTypesCC, ADIOS2BPWriteTypes)
{
    int rank = 0;
    int size = 1;

#ifdef ADIOS2_HAVE_MPI
    adios2_adios *adiosH = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    adios2_adios *adiosH = adios2_init(adios2_debug_mode_on);
#endif

    // write
    {
        // IO
        adios2_io *ioH = adios2_declare_io(adiosH, "CArrayTypes");
        // Set engine parameters
        adios2_set_engine(ioH, "BPFile");
        adios2_set_parameter(ioH, "ProfileUnits", "Microseconds");
        adios2_set_parameter(ioH, "Threads", "1");

        // Set transport and parameters
        size_t transportID;
        adios2_add_transport(&transportID, ioH, "File");
        adios2_set_transport_parameter(ioH, transportID, "library", "fstream");

        // count dims are allocated in stack
        size_t shape[1];
        shape[0] = data_Nx * static_cast<size_t>(size);

        size_t start[1];
        start[0] = data_Nx * static_cast<size_t>(rank);

        size_t count[1];
        count[0] = data_Nx;

        // define attribute
        const char *strarray[] = {"first", "second", "third", "fourth"};

        adios2_define_attribute_array(ioH, "strarray", adios2_type_string,
                                      strarray, 4);

        adios2_define_attribute(ioH, "strvalue", adios2_type_string,
                                "Hello Attribute");

        // Define variables in ioH
        {
            adios2_define_variable(ioH, "varI8", adios2_type_int8_t, 1, shape,
                                   start, count, adios2_constant_dims_true);
            adios2_define_variable(ioH, "varI16", adios2_type_int16_t, 1, shape,
                                   start, count, adios2_constant_dims_true);
            adios2_define_variable(ioH, "varI32", adios2_type_int32_t, 1, shape,
                                   start, count, adios2_constant_dims_true);
            adios2_define_variable(ioH, "varI64", adios2_type_int64_t, 1, shape,
                                   start, count, adios2_constant_dims_true);

            adios2_define_variable(ioH, "varU8", adios2_type_uint8_t, 1, shape,
                                   start, count, adios2_constant_dims_true);
            adios2_define_variable(ioH, "varU16", adios2_type_uint16_t, 1,
                                   shape, start, count,
                                   adios2_constant_dims_true);
            adios2_define_variable(ioH, "varU32", adios2_type_uint32_t, 1,
                                   shape, start, count,
                                   adios2_constant_dims_true);
            adios2_define_variable(ioH, "varU64", adios2_type_uint64_t, 1,
                                   shape, start, count,
                                   adios2_constant_dims_true);

            adios2_define_variable(ioH, "varR32", adios2_type_float, 1, shape,
                                   start, count, adios2_constant_dims_true);
            adios2_define_variable(ioH, "varR64", adios2_type_double, 1, shape,
                                   start, count, adios2_constant_dims_true);
        }
        // inquire variables
        adios2_variable *varI8 = adios2_inquire_variable(ioH, "varI8");
        adios2_variable *varI16 = adios2_inquire_variable(ioH, "varI16");
        adios2_variable *varI32 = adios2_inquire_variable(ioH, "varI32");
        adios2_variable *varI64 = adios2_inquire_variable(ioH, "varI64");

        adios2_variable *varU8 = adios2_inquire_variable(ioH, "varU8");
        adios2_variable *varU16 = adios2_inquire_variable(ioH, "varU16");
        adios2_variable *varU32 = adios2_inquire_variable(ioH, "varU32");
        adios2_variable *varU64 = adios2_inquire_variable(ioH, "varU64");

        adios2_variable *varR32 = adios2_inquire_variable(ioH, "varR32");
        adios2_variable *varR64 = adios2_inquire_variable(ioH, "varR64");
        adios2_engine *engineH =
            adios2_open(ioH, "ctypes.bp", adios2_mode_write);

        adios2_put(engineH, varI8, data_I8, adios2_mode_deferred);
        adios2_put(engineH, varI16, data_I16, adios2_mode_deferred);
        adios2_put(engineH, varI32, data_I32, adios2_mode_deferred);
        adios2_put(engineH, varI64, data_I64, adios2_mode_deferred);

        adios2_put(engineH, varU8, data_U8, adios2_mode_deferred);
        adios2_put(engineH, varU16, data_U16, adios2_mode_deferred);
        adios2_put(engineH, varU32, data_U32, adios2_mode_deferred);
        adios2_put(engineH, varU64, data_U64, adios2_mode_deferred);

        adios2_put(engineH, varR32, data_R32, adios2_mode_deferred);
        adios2_put(engineH, varR64, data_R64, adios2_mode_deferred);

        adios2_close(engineH);
    }
#ifdef ADIOS2_HAVE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    {
        adios2_io *ioH = adios2_declare_io(adiosH, "Reader");
        adios2_engine *engineH =
            adios2_open(ioH, "ctypes.bp", adios2_mode_read);

        adios2_bool result;
        char name[30];
        size_t name_size;
        adios2_type type;
        char type_str[30];
        size_t type_str_size;

        // single
        adios2_attribute *attrSingle =
            adios2_inquire_attribute(ioH, "strvalue");
        adios2_attribute_is_value(&result, attrSingle);
        EXPECT_EQ(result, adios2_true);

        adios2_attribute_name(name, &name_size, attrSingle);
        const std::string nameStrSingle(name, name_size);
        EXPECT_EQ(nameStrSingle, "strvalue");

        adios2_attribute_type(&type, attrSingle);
        EXPECT_EQ(type, adios2_type_string);

        adios2_attribute_type_string(type_str, &type_str_size, attrSingle);
        const std::string typeStrSingle(type_str, type_str_size);
        EXPECT_EQ(typeStrSingle, "string");

        char *dataSingle = new char[30]();
        size_t elements;
        adios2_attribute_data(dataSingle, &elements, attrSingle);
        const std::string dataSingleStr(dataSingle);
        EXPECT_EQ(dataSingleStr, "Hello Attribute");
        EXPECT_EQ(elements, 1);
        delete[] dataSingle;

        // Array
        adios2_attribute *attrArray = adios2_inquire_attribute(ioH, "strarray");
        adios2_attribute_is_value(&result, attrArray);
        EXPECT_EQ(result, adios2_false);

        adios2_attribute_name(name, &name_size, attrArray);
        const std::string nameStrArray(name, name_size);
        EXPECT_EQ(nameStrArray, "strarray");

        adios2_attribute_type(&type, attrArray);
        EXPECT_EQ(type, adios2_type_string);

        adios2_attribute_type_string(type_str, &type_str_size, attrArray);
        const std::string typeStrArray(type_str, type_str_size);
        EXPECT_EQ(typeStrArray, "string");

        char **dataArray = new char *[4];
        for (auto i = 0; i < 4; ++i)
        {
            dataArray[i] = new char[30]();
        }

        adios2_attribute_data(dataArray, &elements, attrArray);
        const std::vector<std::string> dataVector(dataArray,
                                                  dataArray + elements);
        EXPECT_EQ(dataVector[0], "first");
        EXPECT_EQ(dataVector[1], "second");
        EXPECT_EQ(dataVector[2], "third");
        EXPECT_EQ(dataVector[3], "fourth");
        EXPECT_EQ(elements, 4);

        adios2_close(engineH);

        delete[] dataArray;
    }
    // deallocate adiosH
    adios2_finalize(adiosH);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
