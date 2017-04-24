#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTest.h"

class ADIOS1WriteRead1DTest : public EngineWriteRead1DTest
{
public:
    ADIOS1WriteRead1DTest()
    : EngineWriteRead1DTest("ADIOS1Writer", "ADIOS1WriteRead1DTest.bp")
    {
    }
};

TEST_F(ADIOS1WriteRead1DTest, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

class ADIOS1WriteRead2D2x4Test : public EngineWriteRead2D2x4Test
{
public:
    ADIOS1WriteRead2D2x4Test()
    : EngineWriteRead2D2x4Test("ADIOS1Writer", "ADIOS1WriteRead2D2x4Test.bp")
    {
    }
};

TEST_F(ADIOS1WriteRead2D2x4Test, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

class ADIOS1WriteRead2D4x2Test : public EngineWriteRead2D4x2Test
{
public:
    ADIOS1WriteRead2D4x2Test()
    : EngineWriteRead2D4x2Test("ADIOS1Writer", "ADIOS1WriteRead2D4x2Test.bp")
    {
    }
};

TEST_F(ADIOS1WriteRead2D4x2Test, WriteReadTest) { WriteRead(); }
