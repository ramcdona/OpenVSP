
# Workaround for SC_BUILD_SHARED_LIBS flag.
# Would prefer to set to OFF.  However, it won't build on Mac with
# flag set to OFF -- and it won't build on MSVC with it set to ON.
IF( WIN32 )
    SET( SC_SHARED OFF )
ELSE()
    SET( SC_SHARED ON )
ENDIF()

IF( WIN32 )
    SET( SC_PATCH )
ELSE()
    SET( SC_PATCH PATCH_COMMAND patch -p0 -i ${CMAKE_CURRENT_SOURCE_DIR}/STEPCode.patch )
ENDIF()

ExternalProject_Add( STEPCODE
	URL ${CMAKE_CURRENT_SOURCE_DIR}/stepcode-7dcd6ef3418a.zip
	${SC_PATCH}
	CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DSC_BUILD_TYPE=Debug
		-DSC_BUILD_SCHEMAS=ap203/ap203.exp
		-DSC_BUILD_STATIC_LIBS=ON
		-DSC_BUILD_SHARED_LIBS=${SC_SHARED}
		-DSC_PYTHON_GENERATOR=OFF
		-DSC_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)
ExternalProject_Get_Property( STEPCODE SOURCE_DIR )
ExternalProject_Get_Property( STEPCODE BINARY_DIR )
ExternalProject_Get_Property( STEPCODE INSTALL_DIR )

IF( NOT WIN32 )
	SET( STEPCODE_INSTALL_DIR ${SOURCE_DIR}/../sc-install )
ELSE()
	SET( STEPCODE_INSTALL_DIR ${INSTALL_DIR} )
ENDIF()

SET( STEPCODE_BINARY_DIR ${BINARY_DIR} )

# SC CMake does not honor -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
# Consequently, force Debug so it installs in ../sc-install directory
# instead of /usr/local/lib.
#
# SC's own programs fail to build with -DSC_BUILD_SHARED_LIBS=OFF

# The targets below are needed to ensure that the build system
# is aware of the dependency of the library files on the external project.
# This is necessary for Ninja to successfully build the project.

IF(NOT WIN32)
    ADD_CUSTOM_COMMAND(
        OUTPUT
            ${STEPCODE_INSTALL_DIR}/lib/libbase.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepcore.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepeditor.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepdai.a
            ${STEPCODE_INSTALL_DIR}/lib/libsteputils.a
            ${STEPCODE_INSTALL_DIR}/lib/libsdai_ap203.a
    )
    ADD_CUSTOM_TARGET(
        STEPCODE_INTERNAL
        DEPENDS
            STEPCODE
            ${STEPCODE_INSTALL_DIR}/lib/libbase.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepcore.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepeditor.a
            ${STEPCODE_INSTALL_DIR}/lib/libstepdai.a
            ${STEPCODE_INSTALL_DIR}/lib/libsteputils.a
            ${STEPCODE_INSTALL_DIR}/lib/libsdai_ap203.a
    )
ELSE()
    ADD_CUSTOM_COMMAND(
        OUTPUT
        ${STEPCODE_INSTALL_DIR}/lib/libbase.lib
        ${STEPCODE_INSTALL_DIR}/lib/libstepcore.lib
        ${STEPCODE_INSTALL_DIR}/lib/libstepeditor.lib
        ${STEPCODE_INSTALL_DIR}/lib/libstepdai.lib
        ${STEPCODE_INSTALL_DIR}/lib/libsteputils.lib
        ${STEPCODE_INSTALL_DIR}/lib/libsdai_ap203.lib
        ${STEPCODE_INSTALL_DIR}/lib/libexpress.lib
        ${STEPCODE_INSTALL_DIR}/lib/libexppp.lib
    )
    ADD_CUSTOM_TARGET(
        STEPCODE_INTERNAL
        DEPENDS
            STEPCODE
            ${STEPCODE_INSTALL_DIR}/lib/libbase.lib
            ${STEPCODE_INSTALL_DIR}/lib/libstepcore.lib
            ${STEPCODE_INSTALL_DIR}/lib/libstepeditor.lib
            ${STEPCODE_INSTALL_DIR}/lib/libstepdai.lib
            ${STEPCODE_INSTALL_DIR}/lib/libsteputils.lib
            ${STEPCODE_INSTALL_DIR}/lib/libsdai_ap203.lib
            ${STEPCODE_INSTALL_DIR}/lib/libexpress.lib
            ${STEPCODE_INSTALL_DIR}/lib/libexppp.lib
    )
ENDIF()
