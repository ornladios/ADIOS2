#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTestBase.h"

class ADIOS1WriteReadTest : public EngineWriteReadTestBase
{
public:
    ADIOS1WriteReadTest() : EngineWriteReadTestBase() {}

protected:
    void OpenWrite(std::string fname)
    {
        auto &m_EngineSettings = m_adios.DeclareMethod("TestMethod");
        m_EngineSettings.SetEngine("ADIOS1Writer");
        m_EngineSettings.SetParameters("profile_units=mus");
        m_EngineSettings.AddTransport("File", "profile_units=mus",
                                      "have_metadata_file=no");

        m_Engine = m_adios.Open(fname, "w", m_EngineSettings);
        ASSERT_NE(m_Engine, nullptr);
    }
};

TEST_F(ADIOS1WriteReadTest, WriteRead_1D_8)
{
    EngineWriteReadTestBase::Declare1D_8();
    EngineWriteReadTestBase::OpenWriteClose("ADIOS1WriteReadTest_1D_8.bp");
}

TEST_F(ADIOS1WriteReadTest, WriteRead_2D_2x4)
{
    EngineWriteReadTestBase::Declare2D_2x4();
    EngineWriteReadTestBase::OpenWriteClose("ADIOS1WriteReadTest_2D_2x4.bp");
}

TEST_F(ADIOS1WriteReadTest, WriteRead_2D_4x2)
{
    EngineWriteReadTestBase::Declare2D_4x2();
    EngineWriteReadTestBase::OpenWriteClose("ADIOS1WriteReadTest_2D_4x2.bp");
}
