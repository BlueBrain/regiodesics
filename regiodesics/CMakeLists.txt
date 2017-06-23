
set(REGIODESICS_SOURCES Bricks.cpp algorithm.cpp main.cpp)
set(REGIODESICS_HEADERS Volume.h Bricks.h programs.h)

set(REGIODESICS_LINK_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES} ${Boost_LIBRARIES}
  ${OPENGL_LIBRARIES})
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/nrrd)
common_application(regiodesics)