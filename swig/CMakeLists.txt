# swig/CMakeLists.txt
# CMake configuration for SWIG Python bindings

cmake_minimum_required(VERSION 3.16)

# Option for Python stable ABI (abi3) - builds wheel compatible with Python 3.10+
option(OESELECT_USE_STABLE_ABI "Build with Python stable ABI (abi3) for cross-version compatibility" OFF)

# Find required packages
find_package(SWIG 4.0 REQUIRED)
if(OESELECT_USE_STABLE_ABI)
    # For stable ABI, we need Development.SABIModule (CMake 3.26+) or fall back
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.26")
        find_package(Python3 COMPONENTS Interpreter Development.SABIModule REQUIRED)
    else()
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    endif()
else()
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
endif()

include(${SWIG_USE_FILE})

# Set SWIG properties
set_source_files_properties(oeselect.i PROPERTIES
    CPLUSPLUS ON
    SWIG_MODULE_NAME _oeselect
)

# Set SWIG flags
set(CMAKE_SWIG_FLAGS "-doxygen")

# Create the SWIG library
# Use MODULE type instead of SHARED - on macOS this creates a bundle which is
# required for Python extension modules with Python 3.13+
swig_add_library(oeselect_python
    TYPE MODULE
    LANGUAGE python
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES oeselect.i
)

# Configure stable ABI (abi3) if requested
if(OESELECT_USE_STABLE_ABI)
    # Py_LIMITED_API=0x030A0000 means Python 3.10+ stable ABI
    target_compile_definitions(oeselect_python PRIVATE
        Py_LIMITED_API=0x030A0000
    )
    message(STATUS "Building with Python stable ABI (abi3) for Python 3.10+")
endif()

# Set include directories
target_include_directories(oeselect_python PRIVATE
    ${Python3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific linking
if(APPLE)
    # On macOS, use -undefined dynamic_lookup to allow Python symbols to be
    # resolved at runtime by the Python interpreter that loads the module.
    # This is critical for Python 3.13+ multi-phase initialization.
    # Do NOT link against Python3::Python directly on macOS.
    target_link_libraries(oeselect_python
        PRIVATE
            oeselect
    )
    target_link_options(oeselect_python PRIVATE
        -undefined dynamic_lookup
    )
else()
    # On other platforms, link against Python normally
    target_link_libraries(oeselect_python
        PRIVATE
            oeselect
            Python3::Python
    )
endif()

# Set output name and properties
set_target_properties(oeselect_python PROPERTIES
    OUTPUT_NAME "_oeselect"
    PREFIX ""
)

# Platform-specific suffix
if(APPLE)
    set_target_properties(oeselect_python PROPERTIES SUFFIX ".so")
elseif(WIN32)
    set_target_properties(oeselect_python PROPERTIES SUFFIX ".pyd")
endif()

# Add RPATH for finding libraries
if(APPLE)
    if(OpenEye_LIBRARY_TYPE STREQUAL "SHARED")
        # When using shared OpenEye libraries, set rpath to find libraries
        # relative to the openeye package location in site-packages
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "from openeye import libs; import os; print(os.path.basename(libs.FindOpenEyeDLLSDirectory()))"
            OUTPUT_VARIABLE OPENEYE_PLATFORM
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set_target_properties(oeselect_python PROPERTIES
            INSTALL_RPATH "@loader_path;@loader_path/../openeye/libs/${OPENEYE_PLATFORM}"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        message(STATUS "Setting rpath for OpenEye Python package: @loader_path/../openeye/libs/${OPENEYE_PLATFORM}")
    else()
        # Static linking - just need @loader_path for bundled libraries
        set_target_properties(oeselect_python PROPERTIES
            INSTALL_RPATH "@loader_path"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
elseif(UNIX)
    if(OpenEye_LIBRARY_TYPE STREQUAL "SHARED")
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "from openeye import libs; import os; print(os.path.basename(libs.FindOpenEyeDLLSDirectory()))"
            OUTPUT_VARIABLE OPENEYE_PLATFORM
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set_target_properties(oeselect_python PROPERTIES
            INSTALL_RPATH "$ORIGIN:$ORIGIN/../openeye/libs/${OPENEYE_PLATFORM}"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    else()
        set_target_properties(oeselect_python PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
endif()

# Define output directory for Python module
set(PYTHON_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/python/oeselect_lib)

# Custom command to copy the generated Python wrapper and shared library
add_custom_command(TARGET oeselect_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/_oeselect.py
        ${PYTHON_OUTPUT_DIR}/oeselect.py
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:oeselect_python>
        ${PYTHON_OUTPUT_DIR}/$<TARGET_FILE_NAME:oeselect_python>
    COMMENT "Copying Python module to ${PYTHON_OUTPUT_DIR}"
)

# Also copy liboeselect for standalone package
add_custom_command(TARGET oeselect_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:oeselect>
        ${PYTHON_OUTPUT_DIR}/$<TARGET_FILE_NAME:oeselect>
    COMMENT "Copying liboeselect to ${PYTHON_OUTPUT_DIR}"
)

# Install targets for wheel building
# scikit-build-core will set CMAKE_INSTALL_PREFIX appropriately
install(TARGETS oeselect_python
    LIBRARY DESTINATION oeselect_lib
    RUNTIME DESTINATION oeselect_lib
)

# Install the SWIG-generated Python file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_oeselect.py
    DESTINATION oeselect_lib
    RENAME oeselect.py
)

# Install the Python package __init__.py
install(FILES ${CMAKE_SOURCE_DIR}/python/oeselect_lib/__init__.py
    DESTINATION oeselect_lib
)

# Generate build info file with OpenEye version for runtime checking
if(DEFINED OPENEYE_TOOLKITS_VERSION AND OPENEYE_TOOLKITS_VERSION)
    set(_BUILD_INFO_VERSION "${OPENEYE_TOOLKITS_VERSION}")
else()
    set(_BUILD_INFO_VERSION "${OpenEye_VERSION}")
endif()
set(BUILD_INFO_CONTENT "# Auto-generated build information - do not edit
OPENEYE_BUILD_VERSION = \"${_BUILD_INFO_VERSION}\"
OPENEYE_LIBRARY_TYPE = \"${OpenEye_LIBRARY_TYPE}\"
")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/_build_info.py "${BUILD_INFO_CONTENT}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_build_info.py
    DESTINATION oeselect_lib
)
