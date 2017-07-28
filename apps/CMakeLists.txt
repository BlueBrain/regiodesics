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
