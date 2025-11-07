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
include(ADIOSFunctions)

set (TestSets Attribute Common)

add_executable(archive_utility archive_utility.c)

find_program(ZIP_EXECUTABLE NAMES zip PATHS /usr/bin /usr/local/bin /opt/local/bin DOC "Path to the zip executable")
if(ZIP_EXECUTABLE)
    message(STATUS "Found zip executable: ${ZIP_EXECUTABLE}")
else()
    message(NOTICE "Zip executable not found, no backwards compatiblity checks will be performed.")
endif()

foreach (TestSet ${TestSets})

    add_executable(Read${TestSet} Read${TestSet}.cpp)
    target_link_libraries(Read${TestSet} adios2::cxx adios2::c adios2_core adios2::thirdparty::gtest)
    if(ADIOS2_HAVE_MPI)
        add_executable(Write${TestSet} Write${TestSet}.cpp)
        target_link_libraries(Write${TestSet} adios2::cxx_mpi adios2::c_mpi adios2_core_mpi MPI::MPI_CXX adios2::thirdparty::gtest)
    endif()

    file(GLOB ${TestSet}S ${CMAKE_CURRENT_SOURCE_DIR}/zipped_output/${TestSet}*.zip)
    set (OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    list(REMOVE_DUPLICATES ${TestSet}S)

    foreach (${TestSet}OUT ${${TestSet}S})
        get_filename_component(NO_ZIP ${${TestSet}OUT} NAME_WLE)
	get_filename_component(UNIQNAME ${NO_ZIP} NAME)
	#
	# Note:  cmake tar extracts to the current working directory.
	# SetupTestPipeline changes the WD to ${CMAKE_CURRENT_BINARY_DIR}/<pipelinename>
	# So we have to use that as the source of the read.
	#
        add_test(NAME Archive.${UNIQNAME}.Extract
            COMMAND ${CMAKE_COMMAND} -E tar xv "${${TestSet}OUT}"
	)
        add_test(NAME Archive.${UNIQNAME}.Read COMMAND Read${TestSet} "${CMAKE_CURRENT_BINARY_DIR}/Archive.${UNIQNAME}")
	SetupTestPipeline(Archive.${UNIQNAME} "Extract;Read" TRUE)
    endforeach()


    # The code below is for creating "new" output and failing a test
    # if it's unique so that it gets added to the test set.  This is not
    # currently enabled.
    #
    set(NewFileEngineList )
    if (ADIOS2_HAVE_MPI AND ZIP_EXECUTABLE AND NOT WIN32)
        #  Engines to create new files with  (need MPI, ZIP and touch (so don't do on Windows)
	set(NewFileEngineList bp5)
    endif()
    
    foreach	(Engine ${NewFileEngineList})
        add_test(NAME Archive.${TestSet}.${Engine}.Write
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
		${MPIEXEC_NUMPROC_FLAG} 4 $<TARGET_FILE:Write${TestSet}>
		${Engine} ${OUTPUT_DIRECTORY}/Current${TestSet}.${Engine}
	)
	set_tests_properties(Archive.${TestSet}.${Engine}.Write PROPERTIES PROCESSORS 4)

	add_test(NAME Archive.${TestSet}.${Engine}.RemoveVersionInfo
	    COMMAND archive_utility -make_version_neutral ${OUTPUT_DIRECTORY}/Current${TestSet}.${Engine}
	)

	add_test(NAME Archive.${TestSet}.${Engine}.Zip
	    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${OUTPUT_DIRECTORY}/Current${TestSet}.${Engine}
	  	-DARCHIVE_NAME=${OUTPUT_DIRECTORY}/Current${TestSet}.${Engine}.zip 
		-DZIP_EXECUTABLE=${ZIP_EXECUTABLE}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/create_archive.cmake
	)

	add_test(NAME Archive.${TestSet}.${Engine}.Unique
	    COMMAND archive_utility -test_unique ${OUTPUT_DIRECTORY}/Current${TestSet}.${Engine}.zip ${${TestSet}S}
	)


	SetupTestPipeline(Archive.${TestSet}.${Engine} "Write;RemoveVersionInfo;Zip;Unique" TRUE)
    endforeach()
# 
endforeach()  # over testsets

