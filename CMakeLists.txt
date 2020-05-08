cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Regiodesics VERSION 0.1.2)
set(Regiodesics_VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

set(REGIODESICS_DESCRIPTION "A tool for segment")
set(REGIODESICS_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
include(Common)
common_find_package(OpenSceneGraph SYSTEM REQUIRED COMPONENTS
  osg osgViewer osgGA osgUtil osgManipulator)
common_find_package(Boost SYSTEM REQUIRED COMPONENTS filesystem system
                                                     program_options)
common_find_package(OpenGL REQUIRED)
common_find_package(OpenMP)
common_find_package_post()

add_subdirectory(regiodesics)
add_subdirectory(apps)
