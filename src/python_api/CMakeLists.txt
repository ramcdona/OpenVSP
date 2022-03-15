CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

# include the CTest framework
include(CTest)

# CMake 3.9 and below used 'LIBXML2_LIBRARIES' as the name of
# the cache entry storing the find_library result.  Use the
# value if it was set by the project or user.
if(DEFINED LIBXML2_LIBRARY AND NOT DEFINED LIBXML2_LIBRARIES)
  set(LIBXML2_LIBRARIES ${LIBXML2_LIBRARY})
endif()

IF(SWIG_FOUND AND PYTHONLIBS_FOUND)

    MESSAGE( STATUS "Configuring Python API" )

    INCLUDE_DIRECTORIES( ${VSP_SOURCE_DIR}
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIR}
        ${UTIL_INCLUDE_DIR}
        ${GEOM_CORE_INCLUDE_DIR}
        ${GEOM_API_INCLUDE_DIR}
        ${GUI_AND_DRAW_INCLUDE_DIR}
        ${TRIANGLE_INCLUDE_DIR}
        ${NANOFLANN_INCLUDE_DIR}
        ${CLIPPER_INCLUDE_DIR}
        ${CPPTEST_INCLUDE_DIR}
        ${XMLVSP_INCLUDE_DIR}
        ${CPPTEST_INCLUDE_DIR}
        ${FLTK_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${EIGEN3_INCLUDE_DIR}
        ${CodeEli_INCLUDE_DIRS}
        ${STEPCODE_INCLUDE_DIR}
        ${LIBIGES_INCLUDE_DIR}
     )

    SET(CMAKE_SWIG_FLAGS "")

    SET( SWIG_MODULE_vsp_EXTRA_DEPS
        ${UTIL_INCLUDE_DIR}/Defines.h
        ${GEOM_API_INCLUDE_DIR}/APIDefines.h
        ${GEOM_API_INCLUDE_DIR}/APIErrorMgr.h
        ${GEOM_API_INCLUDE_DIR}/VSP_Geom_API.h
        ${GEOM_CORE_INCLUDE_DIR}/SWIGDefines.h
        ${UTIL_INCLUDE_DIR}/Vec3d.h
    )

    set(I_FILES 
        ../geom_api/vsp.i
        )

    SET_SOURCE_FILES_PROPERTIES(${I_FILES} PROPERTIES CPLUSPLUS ON)

    IF (${CMAKE_VERSION} VERSION_LESS "3.8.0")
        SWIG_ADD_MODULE( vsp python ${I_FILES} )
    ELSE()
        SWIG_ADD_LIBRARY( vsp
        LANGUAGE python
        SOURCES ${I_FILES} )
    ENDIF()

    IF( APPLE )
        SET( PY_LIB "" )
        SET_TARGET_PROPERTIES(_vsp PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" )
    ELSE()
        SET( PY_LIB ${PYTHON_LIBRARIES} )
    ENDIF()

    SWIG_LINK_LIBRARIES( vsp
        geom_api
        geom_core
        cfd_mesh
        xmlvsp
        sixseries
        util
        tritri
        clipper
        Angelscript
        wavedragEL
        pinocchio
        ${CPPTEST_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${WINSOCK_LIBRARIES}
        ${PY_LIB}
        ${CMINPACK_LIBRARIES}
        ${STEPCODE_LIBRARIES}
        ${LIBIGES_LIBRARIES}
        ${TRIANGLE_LIBRARIES}
    )

    CONFIGURE_FILE(packages/openvsp/setup.py.in python/packages/openvsp/setup.py)
    CONFIGURE_FILE(packages/openvsp/conda-recipe/meta.yaml.in python/packages/openvsp/conda-recipe/meta.yaml)

# The above files are created in the BINARY_DIR, but we need them back in the source dir.
# when we configure main.h.in with the version number, we're able to INCLUDE_DIRECTORIES to make
# the resulting main.h available to source files.  Sadly, Python did not fall for this trick.
#    INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/python/packages/openvsp/ )
#    INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/python/packages/openvsp/conda-recipe/ )

    INSTALL( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/packages/ DESTINATION python
            PATTERN setup.py.in EXCLUDE
            PATTERN meta.yaml.in EXCLUDE)

    INSTALL( TARGETS _vsp LIBRARY DESTINATION python/openvsp/openvsp )
    INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/vsp.py DESTINATION python/openvsp/openvsp )
    INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/test.py DESTINATION python/openvsp/openvsp/tests )
    INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/python/packages/openvsp/setup.py DESTINATION python/openvsp)
    INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/python/packages/openvsp/conda-recipe/meta.yaml DESTINATION python/openvsp/conda-recipe)


# Can't point at packages/requirements.txt here because it expects to be run from same directory.
# We could possibly add a packages/CMakeLists.txt and run from there.  Instead, I decided to add
# those items as 'local' packages.  Most seem to have worked fine - but not openvsp itself.

# The openvsp package's setup.py lives in BINARY_DIR while the rest of the files live in SOURCE_DIR.
# This manages to complain / fail either way.
# When pointed at BINARY_DIR, it finds setup.py and then fails with:
#    error: package directory 'openvsp' does not exist
#            ${CMAKE_CURRENT_BINARY_DIR}/python/packages/openvsp
# When pointed at SOURCE_DIR it fails to find setup.py with this message:
#    ERROR: Directory '/Users/ramcdona/Documents/OpenVSP/repo/src/python_api/packages/openvsp' is not installable. Neither 'setup.py' nor 'pyproject.toml' found.
#            ${CMAKE_CURRENT_SOURCE_DIR}/packages/openvsp
# As the only reason the files are configured at build time is to add a version number, dummy Setup.py and meta.yaml
# files are added to the repo to satisfy building for testing.  These should not cause any problems for the generation
# of the normal install files.
    CreateVirtualEnvironment( venv
            REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/packages/utilities
            ${CMAKE_CURRENT_SOURCE_DIR}/packages/degen_geom
            ${CMAKE_CURRENT_SOURCE_DIR}/packages/CHARM
            ${CMAKE_CURRENT_SOURCE_DIR}/packages/openvsp
            ${CMAKE_CURRENT_SOURCE_DIR}/packages/AvlPy
            OUT_PYTHON_EXE PYTHON_VENV_EXE
            OUT_BINARY_DIR PYTHON_VENV_BIN_DIR
            OUT_VENV_DIR PYTHON_VENV_DIR )

    MESSAGE( STATUS "PYTHON_VENV_EXE = ${PYTHON_VENV_EXE}")
    MESSAGE( STATUS "PYTHON_VENV_BIN_DIR = ${PYTHON_VENV_BIN_DIR}")

# Attempt to copy _vsp.so target file to somewhere in virtual environment.  Unfortunately, the destination is too
# complex to code with the information available.
#    add_custom_command(TARGET _vsp POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:_vsp> ${targetfile}
#            )


# Add a false dependency to force creation of the virtual environment.
    add_dependencies( vsp venv )

    ADD_TEST( PyTest ${PYTHON_VENV_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py )

ENDIF(SWIG_FOUND AND PYTHONLIBS_FOUND)
