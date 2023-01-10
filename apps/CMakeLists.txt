add_executable(layer_segmenter layer_segmenter.cpp)
target_link_libraries(layer_segmenter PRIVATE regiodesics)

add_executable(geodesics geodesics.cpp)
target_link_libraries(geodesics PRIVATE regiodesics)

add_executable(direction_vectors direction_vectors.cpp)
target_link_libraries(direction_vectors PRIVATE regiodesics)

add_executable(display_geodesics display_geodesics.cpp)
target_link_libraries(display_geodesics PRIVATE regiodesics)

install(
    TARGETS
        layer_segmenter
        geodesics
        direction_vectors
        display_geodesics
    RUNTIME DESTINATION
        bin)
