cmake_minimum_required(VERSION 2.8)

#-----------------------------------------------------------------
# The project
#-----------------------------------------------------------------
project (bubbles)

set(EXTERN_PATH "" CACHE PATH "External dependencies folder")

if (CMAKE_VERSION VERSION_LESS "3.1")
	set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
else ()
	set (CMAKE_CXX_STANDARD 11)
endif ()


if(CMAKE_SYSTEM MATCHES "Linux*")
	message("Detected system ON_LINUX")
	
	set(SYS_BASIC_LIBS pthread rt)
	set(SYS_DYNAMIC_LOAD_LIB dl)
endif(CMAKE_SYSTEM MATCHES "Linux*")

#-----------------------------------------------------------------
# Global include directories
#-----------------------------------------------------------------
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${EXTERN_PATH}/include/
)

link_directories(
	${EXTERN_PATH}/lib
)
#-----------------------------------------------------------------


#-----------------------------------------------------------------
# Project subdirectories
#-----------------------------------------------------------------
add_subdirectory (server)
add_subdirectory (client)


