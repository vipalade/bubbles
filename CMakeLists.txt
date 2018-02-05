cmake_minimum_required(VERSION 2.8)

#-----------------------------------------------------------------
# The project
#-----------------------------------------------------------------
project (bubbles)

set(EXTERN_PATH "" CACHE PATH "External dependencies folder")#TODO rename to EXTERNAL_PATH

if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_STANDARD 11)
endif ()

set(OPENSSL_ROOT_DIR "${EXTERN_PATH}")

find_package(OpenSSL)

set(SolidFrame_DIR "${EXTERN_PATH}/lib/cmake/SolidFrame" CACHE PATH "SolidFrame CMake configuration dir")
find_package(SolidFrame 3)

if(SolidFrame_FOUND)
    message("SolidFrame found with version: ${SolidFrame_VERSION}")
else()
endif()

if(CMAKE_SYSTEM MATCHES "Linux*")
    message("Detected system ON_LINUX")

    set(SYS_BASIC_LIBS pthread rt)
    set(SYS_DYNAMIC_LOAD_LIB dl)
endif(CMAKE_SYSTEM MATCHES "Linux*")

#-----------------------------------------------------------------
# External projects build
#-----------------------------------------------------------------
include(ExternalProject)

set(snappy_PREFIX ${CMAKE_BINARY_DIR}/external/snappy)

ExternalProject_Add(
    build_snappy
    EXCLUDE_FROM_ALL 1
    PREFIX ${snappy_PREFIX}
    URL https://github.com/google/snappy/archive/1.1.7.tar.gz
    CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/external -DCMAKE_INSTALL_LIBDIR=lib
    LOG_UPDATE ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)

set(SNAPPY_LIB ${CMAKE_BINARY_DIR}/external/lib/libsnappy.a)

#-----------------------------------------------------------------
# Global include directories
#-----------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXTERN_PATH}/include/
    ${CMAKE_CURRENT_BINARY_DIR}/external/include
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


