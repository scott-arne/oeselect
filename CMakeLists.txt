# CMakeLists.txt
cmake_minimum_required(VERSION 3.26)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(oeselect VERSION 1.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(OESELECT_BUILD_TESTS "Build tests" ON)
option(OESELECT_BUILD_PYTHON "Build Python bindings" ON)
option(OESELECT_UNIVERSAL2 "Build universal2 binary for macOS" OFF)
option(OESELECT_USE_STABLE_ABI "Use Python stable ABI" ON)

if(APPLE AND OESELECT_UNIVERSAL2)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

# Dependencies
include(FetchContent)

FetchContent_Declare(
    cmake_openeye
    GIT_REPOSITORY https://github.com/scott-arne/cmake-openeye.git
    GIT_TAG v1.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmake_openeye)
list(APPEND CMAKE_MODULE_PATH "${cmake_openeye_SOURCE_DIR}")

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

if(OPENEYE_USE_SHARED)
    find_package(OpenEyePython REQUIRED)
endif()

find_package(OpenEye REQUIRED)

# Library sources (empty for now, will populate)
set(OESELECT_SOURCES
    src/Selection.cpp
    src/Selector.cpp
    src/Context.cpp
    src/SpatialIndex.cpp
    src/Parser.cpp
    src/Predicate.cpp
    src/Tagger.cpp
    src/ResidueSelector.cpp
    src/CustomPredicates.cpp
)

add_library(oeselect ${OESELECT_SOURCES})

set_target_properties(oeselect PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(oeselect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(ZLIB REQUIRED)

# ZLIB::ZLIB is wrapped in BUILD_INTERFACE because on Windows cmake-openeye
# aliases it onto the FetchContent-built `zlibstatic` target, which is not
# in any install export set. install(EXPORT oeselect-targets) would fail
# at configure time ("requires target zlibstatic that is not in any export
# set"). In-tree consumers (the SWIG wrapper) still see ZLIB::ZLIB via
# the build-tree interface; downstream CMake consumers of the installed
# oeselect-targets must call find_package(ZLIB) themselves.
target_link_libraries(oeselect
    PUBLIC
        OpenEye::OEChem
        $<BUILD_INTERFACE:ZLIB::ZLIB>
    PRIVATE
        taocpp::pegtl
        nanoflann::nanoflann
)

# OEBio and OEGrid are optional targets -- in shared-library builds (e.g. from
# openeye-toolkits Python package) these symbols are already in liboechem, so a
# separate library may not exist. They are required on static Windows builds
# and for the SWIG wrapper's OEScalarGrid references.
if(TARGET OpenEye::OEBio)
    target_link_libraries(oeselect PUBLIC OpenEye::OEBio)
endif()
if(TARGET OpenEye::OEGrid)
    target_link_libraries(oeselect PUBLIC OpenEye::OEGrid)
endif()

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
