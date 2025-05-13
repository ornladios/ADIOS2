#
# For each test set <S>, we expect to find Write<S>.cpp and Read<S>.cpp as programs to be built.
# 
# We also expect in the zipped_output directory that all zip files
# with the prefix <S> are the compressed contents of bpfiles that can
# be read with Read<S>
# 
# Write<S>.cpp is expected to be use MPI and we'll (eventually) try to
# create new files with mpirun -n 4 Write<S> ${ENGINE} ${OUTPUT_FILE}
# 
# Read<S>.cpp should not require MPI, but should be GTest enabled.  We'll run it as Read<S> ${INPUT_FILE}
# 
# We'll generate CTest entries for each zipped file we find in
# zipped_output. We expect the filename to start with "<S>.", where
# <S> is one if the TestSet names in the TestSets variable below.
# Current output files look like <S>.<version info>.<engine>.zip.  But the
# <engine> isn't provided to the reader, and the <version info> isn't
# interpreted.  <Version info> could/should also include relevant
# architecture information, like byte order.
#
SET (TestSets Attribute Common)

ADD_EXECUTABLE(test_file_unique test_file_unique.c)

foreach (TestSet ${TestSets})

    ADD_EXECUTABLE(Read${TestSet} Read${TestSet}.cpp)
    target_link_libraries(Read${TestSet} adios2::cxx11 adios2::c adios2_core adios2::thirdparty::gtest)
    if(ADIOS2_HAVE_MPI)
        ADD_EXECUTABLE(Write${TestSet} Write${TestSet}.cpp)
        target_link_libraries(Write${TestSet} adios2::cxx11_mpi adios2::c_mpi adios2_core_mpi MPI::MPI_CXX adios2::thirdparty::gtest)
    endif()

    FILE(GLOB ${TestSet}S ${CMAKE_CURRENT_SOURCE_DIR}/zipped_output/${TestSet}*.zip)
    set (OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TempOutput")
    file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})
    LIST(REMOVE_DUPLICATES ${TestSet}S)

    foreach (${TestSet}OUT ${${TestSet}S})
        GET_FILENAME_COMPONENT(NO_ZIP ${${TestSet}OUT} NAME_WLE)
    	GET_FILENAME_COMPONENT(DIR ${${TestSet}OUT} DIRECTORY)
	GET_FILENAME_COMPONENT(UNIQNAME ${NO_ZIP} NAME)
        GET_FILENAME_COMPONENT(BASENAME ${NO_ZIP} NAME_WLE)
        GET_FILENAME_COMPONENT(ENGINE_EXT ${NO_ZIP} LAST_EXT)
        STRING(REGEX REPLACE "^\\." "" ENGINE ${ENGINE_EXT})
        set (EXTRACTED_BPFILE "${OUTPUT_DIRECTORY}/${NO_ZIP}")
        file (MAKE_DIRECTORY ${EXTRACTED_BPFILE})
	#   MESSAGE (STATUS "Setting up read test for file ${EXTRACTED_BPFILE}")
        file(ARCHIVE_EXTRACT INPUT ${${TestSet}OUT} DESTINATION ${EXTRACTED_BPFILE})
        ADD_TEST(NAME ReadingOldOutput${UNIQNAME} COMMAND Read${TestSet} ${EXTRACTED_BPFILE})
    endforeach()


    # The code below is for creating "new" output and failing a test
    # if it's unique so that it gets added to teh test set.  This is not
    # currently enabled.
    #
    #  Engines to create new files with
    #    SET(NewFileEngineList bp5)
    SET(NewFileEngineList )

    foreach	(Engine ${NewFileEngineList})
        MESSAGE (STATUS "output for engine ${Engine} and test set ${TestSet}")
        add_custom_target(
		new_${TestSet}_${Engine}_OUTPUT ALL
		COMMAND mpirun -n 4 Write${TestSet} ${Engine} ${CMAKE_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}
		COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine} -DARCHIVE_NAME=${CMAKE_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}.zip -P ${CMAKE_SOURCE_DIR}/create_archive.cmake
		COMMAND test_file_unique ${CMAKE_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}.zip ${${TestSet}S}
		BYPRODUCTS ${CURRENT_BINARY_DIR}/Current${TestSet}.${Engine}.zip)
	add_dependencies(new_${TestSet}_${Engine}_OUTPUT Write${TestSet})
    endforeach()
endforeach()  # over testsets

