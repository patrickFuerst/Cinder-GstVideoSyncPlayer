cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( SimpleSyncedPlayer )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )
get_filename_component( GSTSYNCEDPLAYER_BLOCK_DIR "${CINDER_PATH}/blocks/Cinder-GstVideoSyncPlayer" ABSOLUTE )
get_filename_component( OSC_BLOCK_DIR "${CINDER_PATH}/blocks/OSC" ABSOLUTE )

get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src" ABSOLUTE )

set(CINDER_VERBOSE ON)

# Get the sources.
file( GLOB GSTSYNCEDPLAYER_BLOCK_SRC "${GSTSYNCEDPLAYER_BLOCK_DIR}/src/*.cpp" )
file( GLOB GSTSYNCEDPLAYER_BLOCK_INC "${GSTSYNCEDPLAYER_BLOCK_DIR}/src/*.h" )
file( GLOB OSC_BLOCK_SRCS "${OSC_BLOCK_DIR}/src/*.cpp" )


include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( SRC_FILES
	${SRC_DIR}/SimpleSyncedPlayer.cpp
    ${GSTSYNCEDPLAYER_BLOCK_SRC}
        ${GSTSYNCEDPLAYER_BLOCK_INC}
        ${OSC_BLOCK_SRCS}
)

set( INC_DIR
    ${GSTSYNCEDPLAYER_BLOCK_DIR}/src
        ${OSC_BLOCK_DIR}/src
)


ci_make_app(
	SOURCES     ${SRC_FILES}
    INCLUDES    ${INC_DIR}
    LIBRARIES   "gstnet-1.0"
    CINDER_PATH ${CINDER_PATH}
)
