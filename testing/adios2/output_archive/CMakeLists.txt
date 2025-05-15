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

add_executable(test_file_unique test_file_unique.c)

foreach (TestSet ${TestSets})

    add_executable(Read${TestSet} Read${TestSet}.cpp)
    target_link_libraries(Read${TestSet} adios2::cxx11 adios2::c adios2_core adios2::thirdparty::gtest)
    if(ADIOS2_HAVE_MPI)
        add_executable(Write${TestSet} Write${TestSet}.cpp)
        target_link_libraries(Write${TestSet} adios2::cxx11_mpi adios2::c_mpi adios2_core_mpi MPI::MPI_CXX adios2::thirdparty::gtest)
    endif()

    file(GLOB ${TestSet}S ${CMAKE_CURRENT_SOURCE_DIR}/zipped_output/${TestSet}*.zip)
    set (OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TempOutput")
    file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})
    list(REMOVE_DUPLICATES ${TestSet}S)

    foreach (${TestSet}OUT ${${TestSet}S})
        get_filename_component(NO_ZIP ${${TestSet}OUT} NAME_WLE)
    	get_filename_component(DIR ${${TestSet}OUT} DIRECTORY)
	get_filename_component(UNIQNAME ${NO_ZIP} NAME)
        set (EXTRACTED_BPFILE "${OUTPUT_DIRECTORY}/${NO_ZIP}")
        file (MAKE_DIRECTORY ${EXTRACTED_BPFILE})
        add_test(NAME Archive.${UNIQNAME}.Extract
            COMMAND ${CMAKE_COMMAND} -E tar xv ${${TestSet}OUT}
	    WORKING_DIRECTORY ${EXTRACTED_BPFILE}
	)
        add_test(NAME Archive.${UNIQNAME}.Read COMMAND Read${TestSet} ${EXTRACTED_BPFILE})
	SetupTestPipeline(Archive.${UNIQNAME} "Extract;Read" TRUE)
    endforeach()


    # The code below is for creating "new" output and failing a test
    # if it's unique so that it gets added to the test set.  This is not
    # currently enabled.
    #
    set(NewFileEngineList )
    if(ADIOS2_HAVE_MPI)
        #  Engines to create new files with  nothing if no MPI
        set(NewFileEngineList bp5)
    endif()
    
    foreach	(Engine ${NewFileEngineList})
        add_test(NAME Archive.${TestSet}.${Engine}.Write
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
		${MPIEXEC_NUMPROC_FLAG} 4 $<TARGET_FILE:Write${TestSet}>
		${Engine} ${CMAKE_CURRENT_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}
	)
	set_tests_properties(Archive.${TestSet}.${Engine}.Write PROPERTIES PROCESSORS 4)

	add_test(NAME Archive.${TestSet}.${Engine}.Zip
	    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}
	  	-DARCHIVE_NAME=${CMAKE_CURRENT_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}.zip 
		-P ${CMAKE_CURRENT_SOURCE_DIR}/create_archive.cmake
	)

	add_test(NAME Archive.${TestSet}.${Engine}.Unique
	    COMMAND test_file_unique ${CMAKE_CURRENT_BINARY_DIR}/TempOutput/Current${TestSet}.${Engine}.zip ${${TestSet}S}
	)


	SetupTestPipeline(Archive.${TestSet}.${Engine} "Write;Zip;Unique" TRUE)
    endforeach()
# 
endforeach()  # over testsets

