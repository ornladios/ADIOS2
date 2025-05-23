
message(STATUS "Creating archive from ${SOURCE_DIR} into ${ARCHIVE_NAME}")
file(GLOB files_to_archive "${SOURCE_DIR}/*")
list(TRANSFORM files_to_archive REPLACE "${SOURCE_DIR}/" "" )
list(REMOVE_ITEM files_to_archive "profiling.json")
list(JOIN files_to_archive " " files_no_semi)
execute_process (COMMAND bash -c "touch -d 1970-01-01T00:00:00Z ${files_no_semi}" WORKING_DIRECTORY ${SOURCE_DIR})
execute_process (COMMAND bash -c "rm -f ${SOURCE_DIR}.zip; ${ZIP_EXECUTABLE} -qX ${SOURCE_DIR}.zip ${files_no_semi}" WORKING_DIRECTORY ${SOURCE_DIR})

