set(RIPPLES_SOURCES
  ripples.cpp)

set(RIPPLES_LINK_LIBRARIES Layers ${OPENSCENEGRAPH_LIBRARIES})
include_directories(${PROJECT_SOURCE_DIR}/lib)
common_application(ripples)