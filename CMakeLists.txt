cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Layers VERSION 0.0.0)
set(Layers_VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

set(LAYERS_DESCRIPTION "Blah")
set(LAYERS_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
include(Common)
common_find_package(OpenSceneGraph SYSTEM REQUIRED COMPONENTS osg osgViewer
                                                              osgGA)
common_find_package(Boost SYSTEM REQUIRED COMPONENTS filesystem system
                                                     program_options)
common_find_package_post()

add_subdirectory(layers)
