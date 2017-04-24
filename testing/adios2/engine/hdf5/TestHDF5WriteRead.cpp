#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTestBase.h"

class HDF5WriteReadTest : public EngineWriteReadTestBase
{
public:
    HDF5WriteReadTest() : EngineWriteReadTestBase() {}

protected:
    void OpenWrite(std::string fname)
    {
        auto &m_EngineSettings = m_adios.DeclareMethod("TestMethod");
        m_EngineSettings.SetEngine("HDF5Writer");
        m_EngineSettings.SetParameters("profile_units=mus");
        m_EngineSettings.AddTransport("File", "profile_units=mus",
                                      "have_metadata_file=no");

        m_Engine = m_adios.Open(fname, "w", m_EngineSettings);
        ASSERT_NE(m_Engine, nullptr);
    }
};

TEST_F(HDF5WriteReadTest, WriteRead_1D_8)
{
    EngineWriteReadTestBase::Declare1D_8();
    EngineWriteReadTestBase::OpenWriteClose("HDF5WriteReadTest_1D_8.h5");
}

TEST_F(HDF5WriteReadTest, WriteRead_2D_2x4)
{
    EngineWriteReadTestBase::Declare2D_2x4();
    EngineWriteReadTestBase::OpenWriteClose("HDF5WriteReadTest_2D_2x4.h5");
}

TEST_F(HDF5WriteReadTest, WriteRead_2D_4x2)
{
    EngineWriteReadTestBase::Declare2D_4x2();
    EngineWriteReadTestBase::OpenWriteClose("HDF5WriteReadTest_2D_4x2.h5");
}
