set(LAYERS_SOURCES Bricks.cpp main.cpp)
set(LAYERS_HEADERS Volume.h Bricks.h programs.h)

set(LAYERS_LINK_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES} ${Boost_LIBRARIES})
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/nrrd)
common_application(layers)