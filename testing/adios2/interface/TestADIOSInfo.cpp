/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <set>
#include <string>

#include <adios2/common/ADIOSConfig.h>
#include <adios2/core/Info.h>
#include <gtest/gtest.h>

TEST(ADIOSInterface, info_available_features)
{
    size_t nfeatures = 0;
    const char *const *list_features = nullptr;
    adios2_available_features(&nfeatures, &list_features);

    // BP3, BP4 and BP5 are always present, so there are at least 3 features.
    EXPECT_GE(nfeatures, 3u);
    EXPECT_NE(list_features, nullptr);
    EXPECT_EQ(list_features[nfeatures], nullptr);

    // Distinct entries: a missing comma in the macro concatenates literals.
    std::set<std::string> features;
    for (size_t i = 0; i < nfeatures; ++i)
    {
        ASSERT_NE(list_features[i], nullptr);
        features.insert(list_features[i]);
    }
    EXPECT_EQ(features.count("BP3"), 1u);
    EXPECT_EQ(features.count("BP4"), 1u);
    EXPECT_EQ(features.count("BP5"), 1u);
}

TEST(ADIOSInterface, info_available_engines)
{
    size_t nengines = 0;
    const char *const *list_engines = nullptr;
    adios2_available_engines(&nengines, &list_engines);

    EXPECT_GE(nengines, 1);
    EXPECT_NE(list_engines, nullptr);
    EXPECT_EQ(list_engines[nengines], nullptr);

    std::set<std::string> engines;
    for (size_t i = 0; i < nengines; ++i)
    {
        ASSERT_NE(list_engines[i], nullptr);
        engines.insert(list_engines[i]);
    }
    // Always built in.
    EXPECT_EQ(engines.count("BP3"), 1u);
    EXPECT_EQ(engines.count("BP4"), 1u);
    EXPECT_EQ(engines.count("BP5"), 1u);
    EXPECT_EQ(engines.count("Inline"), 1u);

    // Mirror Info.cpp's guards: a wrong macro name/case there drops the engine.
#ifdef ADIOS2_HAVE_HDF5
    EXPECT_EQ(engines.count("HDF5"), 1u);
#endif
#ifdef ADIOS2_HAVE_SST
    EXPECT_EQ(engines.count("SST"), 1u);
#endif
#ifdef ADIOS2_HAVE_MPI
    EXPECT_EQ(engines.count("SSC"), 1u);
#endif
#ifdef ADIOS2_HAVE_DATAMAN
    EXPECT_EQ(engines.count("DataMan"), 1u);
#endif
#ifdef ADIOS2_HAVE_DATASPACES
    EXPECT_EQ(engines.count("DataSpaces"), 1u);
#endif
#ifdef ADIOS2_HAVE_DAOS
    EXPECT_EQ(engines.count("DAOS"), 1u);
#endif
#ifdef ADIOS2_HAVE_MHS
    EXPECT_EQ(engines.count("MHS"), 1u);
#endif
#ifdef ADIOS2_HAVE_CATALYST
    EXPECT_EQ(engines.count("ParaViewADIOSInSituEngine"), 1u);
#endif
}

TEST(ADIOSInterface, info_available_operators)
{
    size_t noperators = 0;
    const char *const *list_operators = nullptr;
    adios2_available_operators(&noperators, &list_operators);

    EXPECT_GE(noperators, 0);
    EXPECT_NE(list_operators, nullptr);
    EXPECT_EQ(list_operators[noperators], nullptr);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
