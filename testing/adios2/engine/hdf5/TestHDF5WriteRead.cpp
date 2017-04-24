#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTest.h"

class HDF5WriteRead1DTest : public EngineWriteRead1DTest
{
public:
    HDF5WriteRead1DTest()
    : EngineWriteRead1DTest("HDF5Writer", "HDF5WriteRead1DTest.h5")
    {
    }
};

TEST_F(HDF5WriteRead1DTest, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

class HDF5WriteRead2D2x4Test : public EngineWriteRead2D2x4Test
{
public:
    HDF5WriteRead2D2x4Test()
    : EngineWriteRead2D2x4Test("HDF5Writer", "HDF5WriteRead2D2x4Test.h5")
    {
    }
};

TEST_F(HDF5WriteRead2D2x4Test, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

class HDF5WriteRead2D4x2Test : public EngineWriteRead2D4x2Test
{
public:
    HDF5WriteRead2D4x2Test()
    : EngineWriteRead2D4x2Test("HDF5Writer", "HDF5WriteRead2D4x2Test.h5")
    {
    }
};

TEST_F(HDF5WriteRead2D4x2Test, WriteReadTest) { WriteRead(); }
