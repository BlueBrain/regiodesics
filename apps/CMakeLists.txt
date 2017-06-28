set(LAYER_SEGMENTER_SOURCES layer_segmenter.cpp)
set(LAYER_SEGMENTER_LINK_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES}
  ${Boost_LIBRARIES} regiodesics)
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/nrrd  ${PROJECT_SOURCE_DIR}/regiodesics)
common_application(layer_segmenter)
