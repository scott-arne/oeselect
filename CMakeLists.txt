# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(oeselect VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(OESELECT_BUILD_TESTS "Build tests" ON)
option(OESELECT_BUILD_PYTHON "Build Python bindings" ON)
option(OESELECT_UNIVERSAL2 "Build universal2 binary for macOS" OFF)

if(APPLE AND OESELECT_UNIVERSAL2)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Dependencies
find_package(OpenEye REQUIRED)

# Fetch header-only dependencies
include(FetchContent)

FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG 3.2.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pegtl)

FetchContent_Declare(
    nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG v1.5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nanoflann)

if(OESELECT_BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    find_package(SWIG 4.0)
endif()

# Library sources (empty for now, will populate)
set(OESELECT_SOURCES
    src/Selection.cpp
    src/Selector.cpp
    src/Context.cpp
    src/Parser.cpp
    src/Predicate.cpp
    src/Tagger.cpp
)

add_library(oeselect ${OESELECT_SOURCES})

set_target_properties(oeselect PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(oeselect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(oeselect
    PUBLIC
        OpenEye::OEChem
    PRIVATE
        taocpp::pegtl
        nanoflann::nanoflann
)

# Tests
if(OESELECT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Python bindings
if(OESELECT_BUILD_PYTHON AND SWIG_FOUND)
    add_subdirectory(swig)
endif()

# Install
install(TARGETS oeselect
    EXPORT oeselect-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/oeselect DESTINATION include)

install(EXPORT oeselect-targets
    FILE OESelectTargets.cmake
    NAMESPACE OESelect::
    DESTINATION lib/cmake/oeselect
)
