# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)


include_directories(
    src/main/cpp/boringssl/include
    src/main/cpp/solidframe
    src/main/cpp
    ${CMAKE_CURRENT_BINARY_DIR}/src/main/cpp/solidframe
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)
#workarround invalid android.toolchaing.cmake - only for clang
#https://android-review.googlesource.com/#/c/291588/4/build/cmake/android.toolchain.cmake

set(ANDROID_ASM_COMPILER "${ANDROID_LLVM_TOOLCHAIN_PREFIX}clang${ANDROID_TOOLCHAIN_SUFFIX}")
set(CMAKE_ASM_COMPILER_TARGET ${ANDROID_LLVM_TRIPLE})
set(CMAKE_ASM_COMPILER_EXTERNAL_TOOLCHAIN "${ANDROID_TOOLCHAIN_ROOT}")
set(CMAKE_ASM_FLAGS ""
        CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_ASM_FLAGS_DEBUG ""
        CACHE STRING "Flags used by the compiler during debug builds.")
set(CMAKE_ASM_FLAGS_RELEASE ""
        CACHE STRING "Flags used by the compiler during release builds.")
set(CMAKE_ASM_FLAGS           "${ANDROID_COMPILER_FLAGS} ${CMAKE_ASM_FLAGS}")
set(CMAKE_ASM_FLAGS_DEBUG     "${ANDROID_COMPILER_FLAGS_DEBUG} ${CMAKE_ASM_FLAGS_DEBUG}")
set(CMAKE_ASM_FLAGS_RELEASE   "${ANDROID_COMPILER_FLAGS_RELEASE} ${CMAKE_ASM_FLAGS_RELEASE}")

add_subdirectory(src/main/cpp/boringssl)
add_subdirectory(src/main/cpp/solidframe)
add_subdirectory(src/main/cpp/client/engine)

message("Android ABI: ${ANDROID_ABI}")
message("Android API level: ${ANDROID_NATIVE_API_LEVEL}")
message("Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message("Processor: ${CMAKE_SYSTEM_PROCESSOR}")


#-----------------------------------------------------------------
# External projects build
#-----------------------------------------------------------------
include(ExternalProject)

set(native_snappy_PREFIX ${CMAKE_BINARY_DIR}/external/snappy)

ExternalProject_Add(
    native_build_snappy
    EXCLUDE_FROM_ALL 0
    BUILD_ALWAYS 1
    PREFIX ${native_snappy_PREFIX}
    URL https://github.com/google/snappy/archive/1.1.7.tar.gz
    CMAKE_COMMAND ${CMAKE_COMMAND}
    CMAKE_GENERATOR "Android Gradle - Ninja"
    CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
    CMAKE_GENERATOR_TOOLSET "${CMAKE_GENERATOR_TOOLSET}"
    CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/external -DCMAKE_INSTALL_LIBDIR=lib
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            -DANDROID_NDK=${ANDROID_NDK}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DANDROID_ARM_NEON=${ANDROID_ARM_NEON}
            -DANDROID_STL=${ANDROID_STL}
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libsnappy.a
    LOG_UPDATE ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)

set(NATIVE_SNAPPY_LIB ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libsnappy.a)

message("snappy_library: ${NATIVE_SNAPPY_LIB}")
message("generator platform: ${CMAKE_GENERATOR_PLATFORM}")
message("generator toolset: ${CMAKE_GENERATOR_TOOLSET}")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/external/include
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             MODULE

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             src/main/cpp/native-lib.cpp )

# - do not understand why this dependency is not considered - moved to egine
add_dependencies(native-lib native_build_snappy)

message("snappy_library2: ${NATIVE_SNAPPY_LIB}")

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       bubbles_client_engine
                       solid_frame_mpipc
                       solid_frame_aio
                       solid_frame_aio_openssl
                       solid_frame
                       solid_serialization
                       solid_utility
                       solid_system
                       ${NATIVE_SNAPPY_LIB}
                       crypto
                       ssl
)
