cmake_minimum_required(VERSION 2.8)

#-----------------------------------------------------------------
# The project
#-----------------------------------------------------------------
project (bubbles)

set(EXTERNAL_PATH "" CACHE PATH "External dependencies folder")#TODO rename to EXTERNAL_PATH

if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "--std=c++14 ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_STANDARD 14)
endif ()


string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)

#force a single configuration type on Windows builds
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)

set(OPENSSL_ROOT_DIR "${EXTERNAL_PATH}")

# OpenSSL pacakge

find_package(OpenSSL)

set(SolidFrame_DIR "${EXTERNAL_PATH}/lib/cmake/SolidFrame" CACHE PATH "SolidFrame CMake configuration dir")
find_package(SolidFrame 3)

if(SolidFrame_FOUND)
    message("SolidFrame found with version: ${SolidFrame_VERSION}")
else()
endif()

# boost pacakge

if(WIN32)
    file(GLOB BOOST_PATH "${EXTERNAL_PATH}/include/boost-*/boost/asio.hpp")
    get_filename_component(BOOST_DIR "${BOOST_PATH}" DIRECTORY)
    get_filename_component(BOOST_DIR "${BOOST_DIR}" DIRECTORY)
    message("boost path ${BOOST_DIR}")
    set(Boost_FOUND TRUE)
else()
    set(BOOST_ROOT "${EXTERNAL_PATH}")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME ON)

    find_package(Boost
        COMPONENTS
            system
            filesystem
            date_time
            program_options
            serialization
    )
endif(WIN32)


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
            -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/external -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=release
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --config release --target install
    LOG_UPDATE ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)

if(WIN32)
    set(SNAPPY_LIB ${CMAKE_BINARY_DIR}/external/lib/snappy.lib)
else()
    set(SNAPPY_LIB ${CMAKE_BINARY_DIR}/external/lib/libsnappy.a)
endif()

#-----------------------------------------------------------------
# Global include directories
#-----------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXTERNAL_PATH}/include/
    ${CMAKE_CURRENT_BINARY_DIR}/external/include
    ${BOOST_DIR}
)

link_directories(
    ${EXTERNAL_PATH}/lib
)
#-----------------------------------------------------------------


#-----------------------------------------------------------------
# Project subdirectories
#-----------------------------------------------------------------
add_subdirectory (server)
add_subdirectory (client)


