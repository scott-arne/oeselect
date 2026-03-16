include(OpenEyeSWIG)

openeye_add_swig_module(
    NAME oeselect
    SWIG_FILE oeselect.i
    LINK_LIBS oeselect
    PYTHON_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/python/oeselect
    STABLE_ABI ${OESELECT_USE_STABLE_ABI}
    SWIG_FLAGS "-doxygen"
    EXPECTED_LIB_VARS
        OECHEM_LIBRARY OESYSTEM_LIBRARY OEPLATFORM_LIBRARY
        OEMATH_LIBRARY OEZSTD_LIBRARY
    INIT_PY ${CMAKE_SOURCE_DIR}/python/oeselect/__init__.py
)