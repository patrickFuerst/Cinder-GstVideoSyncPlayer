cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( SimpleServerPlayer )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )
get_filename_component( GSTSYNCEDPLAYER_BLOCK_DIR "${CINDER_PATH}/blocks/Cinder-GstVideoSyncPlayer" ABSOLUTE )
get_filename_component( OSC_BLOCK_DIR "${CINDER_PATH}/blocks/OSC" ABSOLUTE )

get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src" ABSOLUTE )

set(CINDER_VERBOSE ON)

# Get the sources.
file( GLOB GSTSYNCEDPLAYER_BLOCK_SRC "${GSTSYNCEDPLAYER_BLOCK_DIR}/src/GstVideoServer.cpp" )
file( GLOB GSTSYNCEDPLAYER_BLOCK_INC "${GSTSYNCEDPLAYER_BLOCK_DIR}/src/GstVideoServer.h" )
file( GLOB OSC_BLOCK_SRCS "${OSC_BLOCK_DIR}/src/*.cpp" )


include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( SRC_FILES
	${SRC_DIR}/SimpleServerPlayer.cpp
    ${GSTSYNCEDPLAYER_BLOCK_SRC}
    ${GSTSYNCEDPLAYER_BLOCK_INC}
    ${OSC_BLOCK_SRCS}
)

set( INC_DIR
    ${GSTSYNCEDPLAYER_BLOCK_DIR}/src
    ${OSC_BLOCK_DIR}/src
)

find_package(PkgConfig)
pkg_search_module( GST_NET REQUIRED gstreamer-net gstreamer-net-1.0)

ci_make_app(
	SOURCES     ${SRC_FILES}
    INCLUDES    ${INC_DIR}
    LIBRARIES   ${GST_NET_LDFLAGS}
    CINDER_PATH ${CINDER_PATH}
)
