if( NOT TARGET GstVideoSyncedPlayer )
	get_filename_component( GSTVIDEOSYNCEDPLAYER_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )

	list( APPEND GSTVIDEOSYNCEDPLAYER_SOURCES
		${GSTVIDEOSYNCEDPLAYER_SOURCE_PATH}/GstVideoClient.cpp
		${GSTVIDEOSYNCEDPLAYER_SOURCE_PATH}/GstVideoServer.cpp
	)

	add_library( GstVideoSyncedPlayer ${GSTVIDEOSYNCEDPLAYER_SOURCES} )

	include( "${CINDER_PATH}/proj/cmake/modules/FindGStreamer.cmake" )
	find_package(PkgConfig)
	pkg_search_module( GST_NET REQUIRED gstreamer-net gstreamer-net-1.0)

	list( APPEND GSTREAMER_INCLUDE_DIR_SEPERATE
	${GSTREAMER_INCLUDE_DIRS}
	${GST_NET_INCLUDE_DIRS}
	)

	target_include_directories( GstVideoSyncedPlayer PUBLIC ${GSTREAMER_INCLUDE_DIR_SEPERATE} PUBLIC "${GSTVIDEOSYNCEDPLAYER_SOURCE_PATH}" )

	if( NOT TARGET cinder )
		    include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
		    find_package( cinder REQUIRED PATHS
		        "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
		        "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
	endif()


	# Add OSC block as a dependency
	get_filename_component( OSC_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../OSC/proj/cmake" ABSOLUTE )
	find_package( OSC REQUIRED PATHS "${OSC_MODULE_PATH}" )
	add_dependencies( GstVideoSyncedPlayer OSC )
	target_link_libraries( GstVideoSyncedPlayer PUBLIC OSC PRIVATE cinder PRIVATE ${GST_NET_LIBRARIES} )
endif()