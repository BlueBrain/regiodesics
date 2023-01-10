configure_file(version.h.in ${PROJECT_SOURCE_DIR}/regiodesics/version.h)

add_library(regiodesics
    SHARED
        Bricks.cpp algorithm.cpp version.cpp
    )

set_target_properties(regiodesics
    PROPERTIES
    VERSION ${REGIODESICS_VERSION_STRING}
    SOVERSION ${REGIODESICS_VERSION_MAJOR}.${REGIODESICS_VERSION_MINOR}
)

target_link_libraries(regiodesics
    PUBLIC
        ${OPENSCENEGRAPH_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OPENGL_LIBRARIES}
    )

target_include_directories(regiodesics
    PUBLIC
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/nrrd
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

install(
  TARGETS regiodesics
  LIBRARY DESTINATION lib
)

