CMAKE_MINIMUM_REQUIRED(VERSION 3.24)

PROJECT (VSP)

# Use this to see the command that CMake is using to build code
#SET(CMAKE_VERBOSE_MAKEFILE   ON  CACHE BOOL "" FORCE)

# add the OpenVSP specific cmake files to module path
set(CMAKE_MODULE_PATH ${VSP_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

INCLUDE( VSP_Version )

SET( CMAKE_CXX_STANDARD 17 )

IF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "amd64")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF()

IF( VSP_LIBRARY_PATH )
	INCLUDE( ${VSP_LIBRARY_PATH}/VSP_Libraries_Config.cmake )
ENDIF( VSP_LIBRARY_PATH )

IF(WIN32)
    ADD_DEFINITIONS(/D _CRT_SECURE_NO_WARNINGS /D "NOMINMAX" /MP )
ENDIF(WIN32)

IF(WIN32)
  SET( WINSOCK_LIBRARIES
    wsock32
    ws2_32
  )
ELSE()
  SET( WINSOCK_LIBRARIES )
ENDIF()

INCLUDE( Today )
TODAY( VSPDATE )

FIND_PACKAGE( Doxygen )
IF( DOXYGEN_FOUND )
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
    )
    CONFIGURE_FILE(
        ${CMAKE_CURRENT_SOURCE_DIR}/APIReadme.md.in
        ${CMAKE_CURRENT_BINARY_DIR}/APIReadme.md @ONLY
    )
    ADD_CUSTOM_TARGET( doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
ENDIF( DOXYGEN_FOUND )


CONFIGURE_FILE(
      ${VSP_SOURCE_DIR}/vsp/main.h.in
      ${VSP_BINARY_DIR}/main.h
)
INCLUDE_DIRECTORIES( ${VSP_BINARY_DIR} )

INCLUDE( extLibraryFind )

ADD_SUBDIRECTORY( external )

IF( NOT VSP_NO_GRAPHICS )
	SET ( GUI_AND_DRAW_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/gui_and_draw )
	ADD_SUBDIRECTORY( vsp_graphic )
ENDIF()

SET(GEOM_API_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/geom_api)
ADD_SUBDIRECTORY( util )
ADD_SUBDIRECTORY( xmlvsp )
ADD_SUBDIRECTORY( geom_core )
ADD_SUBDIRECTORY( cfd_mesh )

IF( NOT VSP_NO_GRAPHICS )
	ADD_SUBDIRECTORY( gui_and_draw )
ENDIF()

ADD_SUBDIRECTORY( geom_api )

IF("${CMAKE_SYSTEM}" MATCHES "Linux")
    SET( LINUX_LIBS
        pthread
    )
ELSE()
    SET ( LINUX_LIBS )
ENDIF()

SET( VSP_COMMON_LIBRARIES
    cfd_mesh
    xmlvsp
    sixseries
    util
    tritri
    wavedragEL
    stb_image
    ${LINUX_LIBS}
    ${ANGELSCRIPT_LIBRARIES}
    ${CPPTEST_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${WINSOCK_LIBRARIES}
    ${CMINPACK_LIBRARIES}
    ${STEPCODE_LIBRARIES}
    ${LIBIGES_LIBRARIES}
    ${TRIANGLE_LIBRARIES}
    ${DELABELLA_LIBRARIES}
    ${PINOCCHIO_LIBRARIES}
    ${CLIPPER2_LIBRARIES}
    OpenABF::OpenABF
)

SET( VSP_LIBRARIES_API_FIRST
    geom_api
    geom_core
    ${VSP_COMMON_LIBRARIES}
)

SET( VSP_LIBRARIES_CORE_FIRST
    geom_core
    geom_api
    ${VSP_COMMON_LIBRARIES}
)

SET( VSP_JUST_G_LIBRARIES
    gui_interface_g
    gui_and_draw
    vsp_graphic
    glfont2
    ${EXPRPARSE_LIBRARY}
    cartesian
    ${GLEW_LIBRARY}
    fltk::gl
    fltk::images
    ${OPENGL_LIBRARIES}
)

SET( VSP_G_LIBRARIES_API_FIRST
    geom_api_g
    geom_core
    ${VSP_JUST_G_LIBRARIES}
    ${VSP_COMMON_LIBRARIES}
)

SET( VSP_G_LIBRARIES_CORE_FIRST
    geom_core
    geom_api_g
    ${VSP_JUST_G_LIBRARIES}
    ${VSP_COMMON_LIBRARIES}
)

ADD_SUBDIRECTORY( vsp )

IF( NOT VSP_NO_API_WRAPPERS )
    FIND_PACKAGE( SWIG )

    IF( SWIG_FOUND )
        INCLUDE( ${SWIG_USE_FILE})
    ELSE( SWIG_FOUND )
        MESSAGE( "SWIG not found, skipping API configurations" )
    ENDIF( SWIG_FOUND )

    IF( SWIG_FOUND )
        FIND_PACKAGE( PythonLibsNew 3 )
        if ( PYTHONLIBS_FOUND )
            INCLUDE( CreateVirtualEnvironment )
            ADD_SUBDIRECTORY( python_api )
        ELSE(PYTHONLIBS_FOUND)
            MESSAGE( STATUS "Python not found, API will not be compiled" )
        ENDIF( PYTHONLIBS_FOUND )

        # Note that for the MATLAB API to be built successfully, the unreleased
        # MATLAB branch of SWIG must be compiled first. CMAKE must then be pointed
        # the executable using the SWIG_EXECUTABLE entry. Last, MATLAB must be
        # installed. Note, this has only been tested on 64 Bit Windows, so additional
        # modifications may be required for other operating systems. If you find this
        # to be the case, please consider contributing findings to the public repository.

        IF( VSP_ENABLE_MATLAB_API )
            FIND_PACKAGE( Matlab REQUIRED MX_LIBRARY )

            IF(MATLAB_FOUND)
                ADD_SUBDIRECTORY( matlab_api )
            ELSE(MATLAB_FOUND)
                MESSAGE("MATLAB not found... MATLAB API will NOT be built.")
            ENDIF(MATLAB_FOUND)
        ENDIF()

    ENDIF( SWIG_FOUND )
ENDIF( NOT VSP_NO_API_WRAPPERS )

IF( NOT VSP_NO_VSPAERO )
    ADD_SUBDIRECTORY( vsp_aero )
ENDIF()

IF( NOT VSP_NO_HELP )
    ADD_SUBDIRECTORY( help )
ENDIF()

INCLUDE( IdentifyLibs )  # Debugging to confirm which libraries are set.

SET( CPACK_PACKAGE_NAME OpenVSP )
SET( CPACK_PACKAGE_VERSION_MAJOR ${VSPVER_MAJOR} )
SET( CPACK_PACKAGE_VERSION_MINOR ${VSPVER_MINOR} )
SET( CPACK_PACKAGE_VERSION_PATCH ${VSPVER_PATCH} )
SET( CPACK_PACKAGE_DESCRIPTION "Parametric aircraft geometry tool" )
SET( CPACK_PACKAGE_HOMEPAGE_URL "http://www.openvsp.org" )

IF ( NOT DEFINED VSP_CPACK_GEN )
  SET( VSP_CPACK_GEN ZIP )
ENDIF()

IF ( ${VSP_CPACK_GEN} STREQUAL ZIP )
  SET( CPACK_GENERATOR ZIP )
ELSEIF ( ${VSP_CPACK_GEN} STREQUAL DEB )
  SET( CPACK_GENERATOR ${VSP_CPACK_GEN} )
  SET( CPACK_DEB_PACKAGE_NAME openvsp )
  SET( CPACK_DEBIAN_PACKAGE_MAINTAINER "Rob McDonald <rob.a.mcdonald@gmail.com>" )
  SET( CPACK_DEBIAN_PACKAGE_SECTION "science" )
  SET( CPACK_DEBIAN_PACKAGE_PRIORITY "optional" )
  SET( CPACK_DEBIAN_PACKAGE_HOMEPAGE ${CPACK_PACKAGE_HOMEPAGE_URL} )
  SET( CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION} )
  SET( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )  # Use dh_shlibs to generate required shared libraries/dependencies automatically
  SET( CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON )
  SET( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${VSP_SOURCE_DIR}/cmake/postinst;${VSP_SOURCE_DIR}/cmake/postrm")
  IF( NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX )
    SET( CPACK_PACKAGING_INSTALL_PREFIX /opt/OpenVSP )
  ENDIF()
  SET( CPACK_STRIP_FILES true )
  SET( CMAKE_INSTALL_SO_NO_EXE false )
ELSEIF ( ${VSP_CPACK_GEN} STREQUAL RPM )
  SET( CPACK_GENERATOR ${VSP_CPACK_GEN} )
  IF( NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX )
    SET( CPACK_PACKAGING_INSTALL_PREFIX /opt/OpenVSP )
  ENDIF()
  SET( CPACK_STRIP_FILES true )
ENDIF()

IF( APPLE )
     SET( CPACK_SYSTEM_NAME MacOS )
ENDIF( APPLE )

INCLUDE( CPack )

INSTALL( DIRECTORY ../examples/airfoil/ DESTINATION airfoil )
INSTALL( DIRECTORY ../examples/CustomScripts/ DESTINATION CustomScripts )
INSTALL( DIRECTORY ../examples/matlab/ DESTINATION matlab )
INSTALL( DIRECTORY ../examples/scripts/ DESTINATION scripts )
INSTALL( DIRECTORY ../examples/textures/ DESTINATION textures )
INSTALL( DIRECTORY ../examples/vspaero_ex/ DESTINATION vspaero_ex )
INSTALL( FILES ../LICENSE DESTINATION . )
INSTALL( FILES ../README.md DESTINATION . )
INSTALL( FILES ../CHANGELOG.md DESTINATION . )
INSTALL( FILES ../vspIcon.png DESTINATION .)

# Potentially redundant, but needed if src is used as project root.
INCLUDE( CTest )
ADD_SUBDIRECTORY( test )
