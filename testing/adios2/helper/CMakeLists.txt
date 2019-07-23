#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- #
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- #

add_executable(TestHelperString TestHelperStrings.cpp)
target_link_libraries(TestHelperString adios2 gtest)

gtest_add_tests(TARGET TestHelperString ${extra_test_args})

add_executable(TestDivideBlock TestDivideBlock.cpp)
target_link_libraries(TestDivideBlock adios2 gtest)

gtest_add_tests(TARGET TestDivideBlock ${extra_test_args})


add_executable(TestMinMaxs TestMinMaxs.cpp)
target_link_libraries(TestMinMaxs adios2 gtest)

gtest_add_tests(TARGET TestMinMaxs ${extra_test_args})