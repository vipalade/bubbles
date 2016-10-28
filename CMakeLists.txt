cmake_minimum_required(VERSION 2.8)

#-----------------------------------------------------------------
# The project
#-----------------------------------------------------------------
project (bubbles)

set(EXTERN_PATH "" CACHE PATH "External dependencies folder")

#-----------------------------------------------------------------
# Global include directories
#-----------------------------------------------------------------
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${EXTERN_PATH}/include/
)
#-----------------------------------------------------------------


#-----------------------------------------------------------------
# Project subdirectories
#-----------------------------------------------------------------
add_subdirectory (server)
add_subdirectory (client)

