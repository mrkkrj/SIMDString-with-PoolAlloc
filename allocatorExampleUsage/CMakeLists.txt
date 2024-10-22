if(UNIX)
	cmake_minimum_required(VERSION 3.10)
else()
        cmake_minimum_required(VERSION 3.12)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(SimdStringTest VERSION 0.1.0 LANGUAGES CXX)

option(ExcludeG3dBufferPoolResource "ExcludeG3dBufferPoolResource" OFF)

if(MINGW)
    # MinGW on Windows!
    message( FATAL_ERROR "SIMDString only supports the Microsoft compiler on Windows!" )
endif()

if(CMAKE_HOST_UNIX)
    if(NOT ExcludeG3dBufferPoolResource)
        # GCC support C++17 but not quite all of it! :-OOO
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
            message( STATUS "GCC version: "  ${CMAKE_CXX_COMPILER_VERSION})
            message( FATAL_ERROR "std::memory_resource requires GCC 8.0 and later!" )
        endif()
    endif()
endif()

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Source groups
################################################################################

set(Header_Files
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files__src
    "../src/AllocatorPlatform.h"
    "../src/DebugHelpers.h"
    "../src/g3d_buffer_pool_resource.h"
    "../src/PoolAllocator.h"
    "../src/PoolAllocator.cpp"
)
source_group("Source Files\\src" FILES ${Source_Files__src})

set(Source_Files__simdStrg
    "../simdString/SIMDString.h"
    "../simdString/SIMDString.cpp"
)
source_group("Source Files\\simd_strg" FILES ${Source_Files__simdStrg})

set(Source_Files
    "SimdStringTest.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files__src}
    ${Source_Files__simdStrg}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
if(MSVC)
	# set /SUBSYSTEM:CONSOLE
	add_executable(${PROJECT_NAME} ${ALL_FILES})
else()
	add_executable(${PROJECT_NAME} ${ALL_FILES})
endif()

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../src;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../simdString;"
)

################################################################################
# Compiler flags
################################################################################
if(MSVC)
   # quiet warnings related to fopen, sscanf, etc.
   target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(ExcludeG3dBufferPoolResource)
    add_definitions( -DExcludeG3dBufferPoolResource=1)
endif()

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
)
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
