cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Regiodesics VERSION 0.1.3)
set(Regiodesics_VERSION_ABI 1)

set(REGIODESICS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(REGIODESICS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(REGIODESICS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(REGIODESICS_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgViewer osgGA osgUtil osgManipulator)
find_package(Boost REQUIRED COMPONENTS filesystem system program_options)
find_package(OpenGL REQUIRED)
find_package(OpenMP)

add_subdirectory(regiodesics)
add_subdirectory(apps)
