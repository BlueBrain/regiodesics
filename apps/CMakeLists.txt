include_directories(${PROJECT_SOURCE_DIR}/nrrd ${Boost_INCLUDE_DIRS})

set(LAYER_SEGMENTER_SOURCES layer_segmenter.cpp)
set(LAYER_SEGMENTER_LINK_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES}
  ${Boost_LIBRARIES} regiodesics)
common_application(layer_segmenter)
target_include_directories(layer_segmenter
  PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})

set(GEODESICS_SOURCES geodesics.cpp)
set(GEODESICS_LINK_LIBRARIES ${Boost_LIBRARIES} regiodesics)
common_application(geodesics)

set(DIRECTION_VECTORS_SOURCES direction_vectors.cpp)
set(DIRECTION_VECTORS_LINK_LIBRARIES ${Boost_LIBRARIES} regiodesics)
common_application(direction_vectors)

set(DISPLAY_GEODESICS_SOURCES display_geodesics.cpp)
set(DISPLAY_GEODESICS_LINK_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES} regiodesics
  ${Boost_LIBRARIES})
common_application(display_geodesics)
target_include_directories(display_geodesics
  PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
